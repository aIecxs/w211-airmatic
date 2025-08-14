/*
 * AIRmatic Air Suspension Level Control DIY Kit
 * 
 * Credits:
 * - ArduinoJson (https://github.com/bblanchon/ArduinoJson)
 * - ElegantOTA (https://github.com/ayushsharma82/ElegantOTA)
 * - ESPAsyncWebServer (https://github.com/ESPAsync/ESPAsyncWebServer)
 * - AsyncTCP (https://github.com/ESPAsync/AsyncTCP)
 * - MCP2515 Arduino Library (https://github.com/autowp/arduino-mcp2515)
 * - CAN Bus reverse engineering by @rnd-ash (https://github.com/rnd-ash/mb-w211-pc)
 * 
 * Licensed under the MIT License.
 * Third-party libraries licensed under the LGPL-2.1 License.
 */

#include <SPI.h>
#include <mcp2515.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "w211_can_c.h"
#include "w211_can_b.h"

// reserved
#define LED_BUILTIN GPIO_NUM_2
#define TX0 GPIO_NUM_1 // UART0
#define RX0 GPIO_NUM_3

// MCP2515 CAN Controller Pins
#define SCK  GPIO_NUM_18 // VSPI
#define MISO GPIO_NUM_19
#define MOSI GPIO_NUM_23
#define CS0  GPIO_NUM_5  // Motor CAN-C High speed
#define INT0 GPIO_NUM_34
#define CS1  GPIO_NUM_15 // Interior CAN-B Low speed
#define INT1 GPIO_NUM_35

// TJA1055 CAN-B Transceiver Mode Pins
#define STB GPIO_NUM_21
#define EN  GPIO_NUM_22

// Watchdog
#define WO  GPIO_NUM_32 // TLE4271 Watchdog out

// Enable Pin
#define ON GPIO_NUM_4  // LM2902-N Sensor signal

// Maximum Offset Limit
#define MAX_OFF 30 // mm maximum offset limit / adjust here with caution

// DAC output (offset voltage)
#define PWM1 GPIO_NUM_12 // NVLS1
#define PWM2 GPIO_NUM_13 // NVRS1
#define PWM3 GPIO_NUM_14 // NHRS1
#define PWM4 GPIO_NUM_27 // NHLS1

// CAN IDs Motor CAN-C
#define CANID_0 0x0240 // ECU: EZS, NAME: EZS_240h, ID: 0x0240, MSG COUNT: 31
#define CANID_1 0x0340 // ECU: LF_ABC, NAME: FS_340h, ID: 0x0340, MSG COUNT: 16

// CAN IDs Interior CAN-B
#define CANID_2 0x01CA // ECU: KOMBI, NAME: KOMBI_A5, ID: 0x01CA, MSG COUNT: 25
#define CANID_3 0x001A // ECU: UBF, NAME: UBF_A1, ID: 0x001A, MSG COUNT: 9

// copy CAN_message into bit field decoder
#define copyMsg(ptr) importMsg(#ptr, ptr, sizeof(*(ptr)), id, msg, len)

struct BlinkCmd {
  gpio_num_t pin;
  unsigned int ontime;
};

// CAN Frames Motor CAN-C
struct EZS_240h_t EZS_240h; // ECU: EZS, NAME: EZS_240h, ID: 0x0240, MSG COUNT: 31
struct FS_340h_t FS_340h;   // ECU: LF_ABC, NAME: FS_340h, ID: 0x0340, MSG COUNT: 16

// CAN Frames Interior CAN-B
struct KOMBI_A5_t KOMBI_A5; // ECU: KOMBI, NAME: KOMBI_A5, ID: 0x01CA, MSG COUNT: 25
struct UBF_A1_t UBF_A1;     // ECU: UBF, NAME: UBF_A1, ID: 0x001A, MSG COUNT: 9

// CAN message buffer
struct can_frame canMsg0;
struct can_frame canMsg1;

// Chip select
MCP2515 Can0(CS0); // CS -> GPIO5
MCP2515 Can1(CS1); // CS -> GPIO15

// PWM default value
const int freq = 4000; // 4 kHz
const uint8_t res = 8; // resolution 255
uint8_t duty = 115; // default 45% / adjust here if reference zero position for offset drifts away
int8_t calib_vl = 0; // NVLS1 calibration
int8_t calib_vr = 0; // NVRS1 calibration
int8_t calib_hr = 0; // NHRS1 calibration
int8_t calib_hl = 0; // NHLS1 calibration

String configJson;
String mode = "default";
const char* config = "/config.json";
int8_t offset_nv = 0; // mm front axle level custom offset
int8_t offset_nh = 0; // mm rear axle level custom offset
float factor = (float) duty * 2.0 / 100.0;

volatile unsigned long timer = 0; // millis()

volatile bool canDown = true;
volatile bool canInterruptFlag0 = false;
volatile bool canInterruptFlag1 = false;

portMUX_TYPE mux_can0 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux_can1 = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE mux_awake = portMUX_INITIALIZER_UNLOCKED;

TaskHandle_t canTask0; // Motor CAN-C
TaskHandle_t canTask1; // Interior CAN-B

TaskHandle_t wifiTask;

TaskHandle_t blinkTask;
QueueHandle_t blinkQueue;

// import CAN_message into bit field decoder
void importMsg(const char* name, void* dest, size_t destSize, unsigned int id, const uint8_t* msg, uint8_t len) {
  if (len <= destSize) {
    memcpy(dest, msg, len);
  } else {
    Serial.printf("WARNING: CANID 0x%04X (%s): frame too long, struct size is %d bytes\r\n", id, name, (int)destSize);
    delay(100);
  }
}

// export CAN_message into bit field decoder
void exportMsg(unsigned int id, const uint8_t *msg, uint8_t len)
{
  switch(id) {
    case CANID_0:
      copyMsg(&EZS_240h); // ECU: EZS, NAME: EZS_240h, ID: 0x0240, MSG COUNT: 31
      break;
    case CANID_1:
      copyMsg(&FS_340h); // ECU: LF_ABC, NAME: FS_340h, ID: 0x0340, MSG COUNT: 16
      break;
    case CANID_2:
      copyMsg(&KOMBI_A5); // ECU: KOMBI, NAME: KOMBI_A5, ID: 0x01CA, MSG COUNT: 25
      break;
    case CANID_3:
      copyMsg(&UBF_A1); // ECU: UBF, NAME: UBF_A1, ID: 0x001A, MSG COUNT: 9
      break;
  }
}

// non-blocking delayMicroseconds()
void delay_us(uint32_t us) {
    TaskHandle_t thisTask = xTaskGetCurrentTaskHandle();
    // Timer callback: notify the sleeping task
    auto timerCallback = [](void* arg) {
        TaskHandle_t task = static_cast<TaskHandle_t>(arg);
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(task, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            portYIELD_FROM_ISR();
        }
    };
    // Create one-shot esp_timer
    esp_timer_handle_t timer;
    esp_timer_create_args_t timer_args = {
        .callback = timerCallback,
        .arg = (void*)thisTask,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "delay_us_timer"
    };
    esp_timer_create(&timer_args, &timer);
    esp_timer_start_once(timer, us);  // Delay in microseconds
    // Wait for notification (puts task to sleep)
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    esp_timer_delete(timer);
}

// blink function: non-blocking wrapper
void blinkTaskFunc(void *param) {
  BlinkCmd cmd;
  while (true) {
    if (xQueueReceive(blinkQueue, &cmd, portMAX_DELAY) == pdTRUE) {
      bool state = digitalRead(cmd.pin);
      digitalWrite(cmd.pin, !state);
      delay(cmd.ontime);
      digitalWrite(cmd.pin, state);
    } else {
      delay_us(1000);
    }
  }
}

// blink function
void blink(gpio_num_t pin, unsigned int ontime) {
  BlinkCmd cmd = {pin, ontime};
  // Try to send without blocking, drop if busy
  xQueueSend(blinkQueue, &cmd, 0);
}

// debounce key input
class GetKeyEvent {
private:
  const bool* btn1;
  const bool* btn2;
  bool lastState = false;
  bool stableState = false;
  bool justPressed = false;
  bool repeating = false;
  unsigned long lastChange = 0;
  unsigned long lastRepeat = 0;
  const unsigned long debounceTime = 40;
  const unsigned long repeatDelay = 800;
  const unsigned long repeatInterval = 600;
public:
  GetKeyEvent(const bool* b1, const bool* b2)
    : btn1(b1), btn2(b2) {}
  void read() {
    bool combined = (*btn1) & (*btn2);
    unsigned long time = millis();
    if (combined != lastState) {
      lastState = combined;
      lastChange = time;
    }
    if ((time - lastChange) > debounceTime) {
      if (combined != stableState) {
        stableState = combined;
        if (stableState) {
          justPressed = true;
          lastRepeat = time;
          repeating = false;
        } else {
          repeating = false;
        }
      }
    }
    if (stableState && !repeating && (time - lastRepeat) > repeatDelay) {
      repeating = true;
      lastRepeat = time;
    }
  }
  bool pressed() {
    if (justPressed) {
      justPressed = false;
      return true;
    }
    return false;
  }
  bool repeated() {
    if (repeating) {
      unsigned long time = millis();
      if ((time - lastRepeat) > repeatInterval) {
        lastRepeat = time;
        return true;
      }
    }
    return false;
  }
  bool isDown() const {
    return stableState;
  }
};

// for safety purposes - do not change
void limitOffset(int8_t* off) {
  *off = *off < -MAX_OFF ? -MAX_OFF : *off; // max suspension lowering
  *off = *off > MAX_OFF ? MAX_OFF : *off;   // max suspension height
}

// read config.json
JsonDocument readConfig() {
  JsonDocument doc;
  File file = LittleFS.open(config, "r");
  if (!file) {
    Serial.print("LittleFS: cannot access '");
    Serial.print(config);
    Serial.println("': No such file or directory");
    configJson = "{}";
    return doc;
  }
  DeserializationError err = deserializeJson(doc, file);
  file.seek(0);
  configJson = file.readString();
  file.close();
  if (err) {
    Serial.print("Cannot deserialize the current JSON object: ");
    Serial.println(err.c_str());
  }
  return doc;
}

// update config.json
void saveConfig(const JsonDocument& doc) {
  File file = LittleFS.open(config, "w");
  if (!file) {
    Serial.print("LittleFS: ");
    Serial.print(config);
    Serial.println(": Read-only file system");
    return;
  }
  serializeJson(doc, file);
  file.close();
}

void getCalibration() {
  JsonDocument doc = readConfig();
  calib_vl = 0;
  calib_vr = 0;
  calib_hl = 0;
  calib_hr = 0;
  if (!doc.containsKey("calibration")) return;
  JsonObject obj = doc["calibration"];
  if (obj.containsKey("calib_vl")) calib_vl = obj["calib_vl"];
  if (obj.containsKey("calib_vr")) calib_vr = obj["calib_vr"];
  if (obj.containsKey("calib_hl")) calib_hl = obj["calib_hl"];
  if (obj.containsKey("calib_hr")) calib_hr = obj["calib_hr"];
}

void updateCalibration(const String& key, int8_t value) {
  JsonDocument doc = readConfig();
  JsonObject calibObj;
  if (doc.containsKey("calibration")) {
    calibObj = doc["calibration"];
  } else {
    calibObj = doc.createNestedObject("calibration");
  }
  calibObj[key] = value;
  saveConfig(doc);
}

// read table
void getSettings(const String& mode) {
  JsonDocument doc = readConfig();
  offset_nv = 0;
  offset_nh = 0;
  if (!doc.containsKey(mode)) return;
  JsonObject obj = doc[mode];
  if (obj.containsKey("offset_nv")) offset_nv = obj["offset_nv"];
  if (obj.containsKey("offset_nh")) offset_nh = obj["offset_nh"];
}

// write table
void updateSettings(const String& mode, const String& offset, int8_t value) {
  JsonDocument doc = readConfig();
  JsonObject modeObj;
  if (doc.containsKey(mode)) {
    modeObj = doc[mode];
  } else {
    modeObj = doc.createNestedObject(mode);
  }
  modeObj[offset] = value;
  saveConfig(doc);
}

// Interrupt based CanRx
void IRAM_ATTR onCanInterrupt0() {
  portENTER_CRITICAL_ISR(&mux_can0);
  canInterruptFlag0 = true;
  portEXIT_CRITICAL_ISR(&mux_can0);
}
void IRAM_ATTR onCanInterrupt1() {
  portENTER_CRITICAL_ISR(&mux_can1);
  canInterruptFlag1 = true;
  portEXIT_CRITICAL_ISR(&mux_can1);
}

// power down no CAN traffic
void go_to_sleep(unsigned int timeout) {
  if ( millis() - timer > timeout ) {
    portENTER_CRITICAL(&mux_awake);
    timer = millis();
    canDown = true;
    portEXIT_CRITICAL(&mux_awake);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Can: timeout ... no traffic");
    // put TJA1055 into go-to-sleep
    digitalWrite(STB, LOW);
    delay_us(1000);
    digitalWrite(EN, LOW);
  }
}

// keep awake CAN traffic
void awake(unsigned int delayMs) {
  static unsigned long time = 0;
  bool wakeup = false;
  if ( millis() - time > delayMs ) {
    time = millis();
    portENTER_CRITICAL(&mux_awake);
    timer = time;
    if (canDown) {
      canDown = false;
      wakeup = true;
    }
    portEXIT_CRITICAL(&mux_awake);
    if (wakeup) {
      // put TJA1055 into Normal Mode
      digitalWrite(STB, HIGH);
      digitalWrite(EN, HIGH);
      digitalWrite(LED_BUILTIN, HIGH);
      wakeup = false;
    }
  }
}


void setup() {

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200, SERIAL_8N1, RX0, TX0);
  SPI.begin(SCK, MISO, MOSI);

  /* SPI flash file system
  |--------------|-------|---------------|--|--|--|--|--|
  ^              ^       ^               ^     ^
  Sketch    OTA update   File system   EEPROM  WiFi config (SDK) */
  LittleFS.begin(true);

  // Normal Mode TJA1055
  pinMode(EN, OUTPUT);
  pinMode(STB, OUTPUT);
  digitalWrite(EN, HIGH);
  digitalWrite(STB, HIGH);

  Can0.reset();
  Can1.reset();
  delay_us(1000);

  Can0.setBitrate(CAN_500KBPS, MCP_8MHZ); // Motor CAN-C High speed
  Can1.setBitrate(CAN_83K3BPS, MCP_16MHZ); // Interior CAN-B Low speed

  // Filters for Receive Buffer RXB0 (uses MASK0, filters RXF0 and RXF1)
  Can0.setFilterMask(MCP2515::MASK0, false, 0x7FF); // Standard ID mask = 11 bits
  Can0.setFilter(MCP2515::RXF0, false, CANID_0); // ECU: EZS, NAME: EZS_240h, ID: 0x0240, MSG COUNT: 31
  Can0.setFilter(MCP2515::RXF1, false, CANID_1); // ECU: LF_ABC, NAME: FS_340h, ID: 0x0340, MSG COUNT: 16

  // Filters for Receive Buffer RXB1 (uses MASK1, filters RXF2 to RXF5)
  Can0.setFilterMask(MCP2515::MASK1, false, 0x7FF); // Standard ID mask = 11 bits
  Can0.setFilter(MCP2515::RXF2, false, CANID_0); // ECU: EZS, NAME: EZS_240h, ID: 0x0240, MSG COUNT: 31
  Can0.setFilter(MCP2515::RXF3, false, CANID_1); // ECU: LF_ABC, NAME: FS_340h, ID: 0x0340, MSG COUNT: 16

  // Filters for Receive Buffer RXB0 (uses MASK0, filters RXF0 and RXF1)
  Can1.setFilterMask(MCP2515::MASK0, false, 0x7FF); // Standard ID mask = 11 bits
  Can1.setFilter(MCP2515::RXF0, false, CANID_2); // ECU: KOMBI, NAME: KOMBI_A5, ID: 0x01CA, MSG COUNT: 25
  Can1.setFilter(MCP2515::RXF1, false, CANID_3); // ECU: UBF, NAME: UBF_A1, ID: 0x001A, MSG COUNT: 9

  // Filters for Receive Buffer RXB1 (uses MASK1, filters RXF2 to RXF5)
  Can1.setFilterMask(MCP2515::MASK1, false, 0x7FF); // Standard ID mask = 11 bits
  Can1.setFilter(MCP2515::RXF2, false, CANID_2); // ECU: KOMBI, NAME: KOMBI_A5, ID: 0x01CA, MSG COUNT: 25
  Can1.setFilter(MCP2515::RXF3, false, CANID_3); // ECU: UBF, NAME: UBF_A1, ID: 0x001A, MSG COUNT: 9

  if (Can0.setListenOnlyMode() == MCP2515::ERROR_OK) {
    Serial.println("MCP2515 initialized");
  } else {
    Serial.println("WARNING: MCP2515 not initialized");
  }
  if (Can1.setListenOnlyMode() == MCP2515::ERROR_OK) {
    Serial.println("MCP2515 initialized");
  } else {
    Serial.println("WARNING: MCP2515 not initialized");
  }

  // MCP2515 Interrupts
  pinMode(INT0, INPUT);
  pinMode(INT1, INPUT);
  attachInterrupt(digitalPinToInterrupt(INT0), onCanInterrupt0, FALLING);
  attachInterrupt(digitalPinToInterrupt(INT1), onCanInterrupt1, FALLING);

  // create a task that will be executed along the loop() function, with priority 3 and executed on core 0
  xTaskCreatePinnedToCore(
    canEvent0,     // Task function
    "Can0 Events", // name of task
    4096,          // Stack size of task
    NULL,          // parameter of the task
    3,             // priority of the task
    &canTask0,     // Task handle to keep track of created task
    0);            // pin task to core 0
  
  // create a task that will be executed along the loop() function, with priority 2 and executed on core 1
  xTaskCreatePinnedToCore(
    canEvent1,     // Task function
    "Can1 Events", // name of task
    4096,          // Stack size of task
    NULL,          // parameter of the task
    2,             // priority of the task
    &canTask1,     // Task handle to keep track of created task
    1);            // pin task to core 1

  (void) readConfig();

  // create a task that will be executed along the loop() function, with priority 1
  wifiSetup();
  xTaskCreate(
    wifiEvent,     // Task function
    "WiFi Event",  // name of task
    4096,          // Stack size of task
    NULL,          // parameter of the task
    1,             // priority of the task
    &wifiTask);    // Task handle to keep track of created task

  // create a task that will be executed along the loop() function, with priority 1
  blinkQueue = xQueueCreate(1, sizeof(BlinkCmd));
  xTaskCreate(
    blinkTaskFunc, // Task function
    "blinkTask",   // name of task
    2048,          // Stack size of task
    NULL,          // parameter of the task
    1,             // priority of the task
    &blinkTask);   // Task handle to keep track of created task

  // DAC offset: Vref on
  pinMode(ON, OUTPUT);
  digitalWrite(ON, HIGH);

  // Watchdog output
  pinMode(WO, OUTPUT);

  // DAC offset voltage generator 4 kHz
  ledcAttach(PWM1, freq, res);
  ledcAttach(PWM2, freq, res);
  ledcAttach(PWM3, freq, res);
  ledcAttach(PWM4, freq, res);

  Serial.println("full source code available at -> github.com/aiecxs/w211-airmatic");
}


void loop() {

  static uint8_t ic_stat = 0;
  static String lastMode = mode;
  static unsigned long timeMs = millis();

  // temp buttons: map bitfield bools into regular bools
  static bool button0 = 0; // MSG NAME: BUTTON_4_2 - Telefon End, OFFSET 8, LENGTH 1
  static bool button1 = 0; // MSG NAME: BUTTON_4_1 - Telefon Send, OFFSET 9, LENGTH 1
  static bool button2 = 0; // MSG NAME: BUTTON_3_2 - Taste "-", OFFSET 10, LENGTH 1
  static bool button3 = 0; // MSG NAME: BUTTON_3_1 - Taste "+", OFFSET 11, LENGTH 1
//  static bool button4 = 0; // MSG NAME: BUTTON_2_2 - Reserve, OFFSET 12, LENGTH 1
//  static bool button5 = 0; // MSG NAME: BUTTON_2_1 - Reserve, OFFSET 13, LENGTH 1
  static bool button6 = 0; // MSG NAME: BUTTON_1_2 - Vorheriges Display, OFFSET 14, LENGTH 1
  static bool button7 = 0; // MSG NAME: BUTTON_1_1 - Nächstes Display, OFFSET 15, LENGTH 1

  // declare key combinations
  static GetKeyEvent keyComboRearReset(&button0, &button6);
  static GetKeyEvent keyComboFrontReset(&button0, &button7);
  static GetKeyEvent keyComboRearSet(&button1, &button6);
  static GetKeyEvent keyComboFrontSet(&button1, &button7);
  static GetKeyEvent keyComboRearDn(&button2, &button6);
  static GetKeyEvent keyComboFrontDn(&button2, &button7);
  static GetKeyEvent keyComboRearUp(&button3, &button6);
  static GetKeyEvent keyComboFrontUp(&button3, &button7);

  // main loop
  if (EZS_240h.KL_15) {
    if (FS_340h.FS_ID == 2) {

      // todo: check the KOMBI_A9 IPS Mode (Comfort/Sport)
      mode = "comfort";

    } else if (FS_340h.FS_ID == 1) {
      // check the AIRmatic mode (Offroad/Comfort/Sport1/Sport2)
      if (FS_340h.ST2_LED_DL) {
        mode = "offroad";
      } else if (FS_340h.ST3_LEDR_DL) {
        mode = "sport2";
      } else if (FS_340h.ST3_LEDL_DL) {
        mode = "sport1";
      } else {
        mode = "comfort";
      }
    }
  }

  // read offsets from table
  if (mode != lastMode) {
    lastMode = mode;
    // on table: apply offsets
    getSettings(mode);
    limitOffset(&offset_nv);
    limitOffset(&offset_nh);
    Serial.print("mode = "); Serial.println(mode);
    Serial.print("offset_nv = "); Serial.print(offset_nv, DEC); Serial.println(" mm front axle level custom offset");
    Serial.print("offset_nh = "); Serial.print(offset_nh, DEC); Serial.println(" mm rear axle level custom offset");
    blink(LED_BUILTIN, 100);
  }

  // check display is in phone mode
  if (KOMBI_A5.KI_STAT) {
    ic_stat = KOMBI_A5.KI_STAT;
  }
  if (ic_stat == 5) {
    // get keyevents
    // FRONT + HIGHER
    button7 = KOMBI_A5.BUTTON_1_1;
    button3 = KOMBI_A5.BUTTON_3_1;
    keyComboFrontUp.read();
    if (keyComboFrontUp.pressed() || keyComboFrontUp.repeated()) {
      offset_nv += 5; // increase
      limitOffset(&offset_nv);
      Serial.print("offset_nv = "); Serial.print(offset_nv, DEC); Serial.println(" mm front axle level custom offset");
      blink(LED_BUILTIN, 100);
    }
    // FRONT + LOWER
    button7 = KOMBI_A5.BUTTON_1_1;
    button2 = KOMBI_A5.BUTTON_3_2;
    keyComboFrontDn.read();
    if (keyComboFrontDn.pressed() || keyComboFrontDn.repeated()) {
      offset_nv -= 5; // decrease
      limitOffset(&offset_nv);
      Serial.print("offset_nv = "); Serial.print(offset_nv, DEC); Serial.println(" mm front axle level custom offset");
      blink(LED_BUILTIN, 100);
    }
    // REAR + HIGHER
    button6 = KOMBI_A5.BUTTON_1_2;
    button3 = KOMBI_A5.BUTTON_3_1;
    keyComboRearUp.read();
    if (keyComboRearUp.pressed() || keyComboRearUp.repeated()) {
      offset_nh += 5; // increase
      limitOffset(&offset_nh);
      Serial.print("offset_nh = "); Serial.print(offset_nh, DEC); Serial.println(" mm rear axle level custom offset");
      blink(LED_BUILTIN, 100);
    }
    // REAR + LOWER
    button6 = KOMBI_A5.BUTTON_1_2;
    button2 = KOMBI_A5.BUTTON_3_2;
    keyComboRearDn.read();
    if (keyComboRearDn.pressed() || keyComboRearDn.repeated()) {
      offset_nh -= 5; // decrease
      limitOffset(&offset_nh);
      Serial.print("offset_nh = "); Serial.print(offset_nh, DEC); Serial.println(" mm rear axle level custom offset");
      blink(LED_BUILTIN, 100);
    }
    // on confirm: write offsets to table
    // FRONT + SAVE
    button7 = KOMBI_A5.BUTTON_1_1;
    button1 = KOMBI_A5.BUTTON_4_1;
    keyComboFrontSet.read();
    if (keyComboFrontSet.repeated()) {
      Serial.print("offset_nv = "); Serial.print(offset_nv, DEC); Serial.println(" mm front axle -> SET");
      updateSettings(mode, "offset_nv", offset_nv);
      blink(LED_BUILTIN, 100);
    }
    // REAR + SAVE
    button6 = KOMBI_A5.BUTTON_1_2;
    button1 = KOMBI_A5.BUTTON_4_1;
    keyComboRearSet.read();
    if (keyComboRearSet.repeated()) {
      Serial.print("offset_nh = "); Serial.print(offset_nh, DEC); Serial.println(" mm rear axle -> SET");
      updateSettings(mode, "offset_nh", offset_nh);
      blink(LED_BUILTIN, 100);
    }
    // FRONT + CANCEL
    button7 = KOMBI_A5.BUTTON_1_1;
    button0 = KOMBI_A5.BUTTON_4_2;
    keyComboFrontReset.read();
    if (keyComboFrontReset.pressed()) {
      offset_nv = 0;
      Serial.print("offset_nv = "); Serial.print(offset_nv, DEC); Serial.println(" mm front axle -> SET");
      updateSettings(mode, "offset_nv", offset_nv);
      blink(LED_BUILTIN, 100);
    }
    // REAR + CANCEL
    button6 = KOMBI_A5.BUTTON_1_2;
    button0 = KOMBI_A5.BUTTON_4_2;
    keyComboRearReset.read();
    if (keyComboRearReset.pressed()) {
      offset_nh = 0;
      Serial.print("offset_nh = "); Serial.print(offset_nh, DEC); Serial.println(" mm rear axle -> SET");
      updateSettings(mode, "offset_nh", offset_nh);
      blink(LED_BUILTIN, 100);
    }
  }

  // DAC offset voltage output
  ledcWrite(PWM1, duty + calib_vl + offset_nv * factor); // NVLS1
  ledcWrite(PWM2, duty + calib_vr - offset_nv * factor); // NVRS1 / inverted, requires negative offset
  ledcWrite(PWM3, duty + calib_hr + offset_nh * factor); // NHRS1
//  ledcWrite(PWM4, duty + calib_hl + offset_nh * factor); // NHLS1 / not available for 211/219

  // put TJA1055 into go-to-sleep / TJA1055 does the rest and will switch off TLE4271 automatically
  go_to_sleep(10000); // 10 sec

  // Watchdog output pulse
  if ( millis() - timeMs > 500 ) {
    timeMs = millis();
    digitalWrite(WO, !digitalRead(WO));
  }

  delay(10);
}
