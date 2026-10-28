// debug statistics
typedef struct canStats_t {
  unsigned int rx_count = 0;
  long long last_msg_time = 0;
  unsigned int min_interval_us = 0xFFFFFFFF;
  unsigned int max_interval_us = 0;
  unsigned long long total_interval_us = 0;
  float current_10s_ms = 0.0f;
  static const int WINDOW_SIZE = 500;
  unsigned int deltas[WINDOW_SIZE];
  long long timestamps[WINDOW_SIZE];
  int head = 0;
  int count = 0;
  uint8_t id;
  unsigned long last_stats_time = 0;
  SemaphoreHandle_t mutex;
  void update() {
    long long now = esp_timer_get_time();
    xSemaphoreTake(mutex, portMAX_DELAY);
    if (last_msg_time != 0) {
      unsigned int delta_us = (unsigned int)(now - last_msg_time);
      if (delta_us < 200000) {
        if (delta_us < min_interval_us) min_interval_us = delta_us;
        if (delta_us > max_interval_us) max_interval_us = delta_us;
        total_interval_us += delta_us;
        rx_count++;
        deltas[head] = delta_us;
        timestamps[head] = now;
        head = (head + 1) % WINDOW_SIZE;
        if (count < WINDOW_SIZE) count++;
      }
    }
    last_msg_time = now;
    xSemaphoreGive(mutex);
  }
  void updateWindow() {
    if (millis() - last_stats_time < 1000) {
      return;
    }
    if (xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE) {
      if (millis() - last_stats_time < 1000) {
        xSemaphoreGive(mutex);
        return; 
      }
      last_stats_time = millis();
      long long now_us = esp_timer_get_time();
      float sum = 0;
      int valid_count = 0;
      for (int i = 0; i < count; i++) {
        int idx = (head - 1 - i + WINDOW_SIZE) % WINDOW_SIZE;
        if (now_us - timestamps[idx] <= 10000000LL) {
          sum += deltas[idx] / 1000.0f;
          valid_count++;
        } else {
          break;
        }
      }
      current_10s_ms = (valid_count > 0) ? (sum / valid_count) : 0.0f;
      xSemaphoreGive(mutex);
    }
  }
} canStats_t;

canStats_t can0_stats = { .id = 0 };
canStats_t can1_stats = { .id = 1 };

void initCanStatsMutexes() {
  can0_stats.mutex = xSemaphoreCreateMutex();
  can1_stats.mutex = xSemaphoreCreateMutex();
}

void canEvent0(void *pvParameters) {
  bool rxbox0 = false;
  bool rxbox1 = false;
  if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE) {
    Can0->clearInterrupts();
    xSemaphoreGive(spiMutex);
  }
  while (true) {
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)) > 0) {
      if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE) {
        uint8_t irq = Can0->getInterrupts();
        if (irq & MCP2515::CANINTF_RX0IF) {
          if (Can0->readMessage(MCP2515::RXB0, &canMsg[0]) == MCP2515::ERROR_OK) {
            rxbox0 = true;
          }
        }
        if (irq & MCP2515::CANINTF_RX1IF) {
          if (Can0->readMessage(MCP2515::RXB1, &canMsg[1]) == MCP2515::ERROR_OK) {
            rxbox1 = true;
          }
        }
        Can0->clearInterrupts();
        xSemaphoreGive(spiMutex);
      }
      if (rxbox0 || rxbox1) {
        can0_stats.update();
      }
      if (rxbox0) {
        // export CAN message into bitfield decoder
        exportMsg(canMsg[0].can_id, canMsg[0].data, canMsg[0].can_dlc);
        rxbox0 = false;
      }
      if (rxbox1) {
        // export CAN message into bitfield decoder
        exportMsg(canMsg[1].can_id, canMsg[1].data, canMsg[1].can_dlc);
        rxbox1 = false;
      }
      awake(100); // prevent idle timeout
    }
    if (can0) {
      // send mailbox queue with delay 1 ms
      sendMbx(Can0, &mbox0, 1);
    }
    portYIELD();
  }
}

void canEvent1(void *pvParameters) {
  bool rxbox0 = false;
  bool rxbox1 = false;
  if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE) {
    Can1->clearInterrupts();
    xSemaphoreGive(spiMutex);
  }
  while (true) {
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1)) > 0) {
      if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE) {
        uint8_t irq = Can1->getInterrupts();
        if (irq & MCP2515::CANINTF_RX0IF) {
          if (Can1->readMessage(MCP2515::RXB0, &canMsg[2]) == MCP2515::ERROR_OK) {
            rxbox0 = true;
          }
        }
        if (irq & MCP2515::CANINTF_RX1IF) {
          if (Can1->readMessage(MCP2515::RXB1, &canMsg[3]) == MCP2515::ERROR_OK) {
            rxbox1 = true;
          }
        }
        Can1->clearInterrupts();
        xSemaphoreGive(spiMutex);
      }
      if (rxbox0 || rxbox1) {
        can1_stats.update();
      }
      if (rxbox0) {
        // export CAN message into bitfield decoder
        exportMsg(canMsg[2].can_id, canMsg[2].data, canMsg[2].can_dlc);
        rxbox0 = false;
      }
      if (rxbox1) {
        // export CAN message into bitfield decoder
        exportMsg(canMsg[3].can_id, canMsg[3].data, canMsg[3].can_dlc);
        rxbox1 = false;
      }
      awake(100); // prevent idle timeout
    }
    // send mailbox queue with delay 1 ms
    if (can1) {
      sendMbx(Can1, &mbox1, 1);
    }
    portYIELD();
  }
}

// Interrupt based CanRx
void IRAM_ATTR onCanInterrupt0() {
  if (canTask0 != nullptr) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(canTask0, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}
void IRAM_ATTR onCanInterrupt1() {
  if (canTask1 != nullptr) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(canTask1, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

bool enqueueFrame(mbx_t *mbox, const struct can_frame *msg) {
  if (mbox->count < MAILBOX_CAPACITY) {
    mbox->frames[mbox->tail] = *msg;
    mbox->tail = (mbox->tail + 1) % MAILBOX_CAPACITY;
    mbox->count++;
    return true;
  } else {
    return false;
  }
}

bool dequeueFrame(mbx_t *mbox, struct can_frame *msg) {
  if (mbox->count) {
    *msg = mbox->frames[mbox->head];
    mbox->head = (mbox->head + 1) % MAILBOX_CAPACITY;
    mbox->count--;
    return true;
  } else {
    return false;
  }
}

void sendMsg(mbx_t *mbox, canid_t id, const void *msg, size_t len, uint8_t repeat) {
  struct can_frame buf;
  buf.can_id  = id;
  buf.can_dlc = len > CAN_MAX_DLEN ? CAN_MAX_DLEN : len;
  memcpy(buf.data, msg, buf.can_dlc);
  for (uint8_t i = 0; i < repeat; i++) {
    if (!enqueueFrame(mbox, &buf)) {
      Serial.printf(
        "Message not sent: CanId 0x%04X: send mailbox capacity exceeded!\n",
        buf.can_id
      );
      delay(100);
    }
  }
}

void sendMbx(MCP2515 *can, mbx_t *mbox, unsigned long min_interval_ms) {
  unsigned long now = millis();
  if (mbox->count) {
    if (now - mbox->timestamp > min_interval_ms) {
      struct can_frame msg;
      if (dequeueFrame(mbox, &msg)) {
        mbox->timestamp = now;
        if (xSemaphoreTake(spiMutex, portMAX_DELAY) == pdTRUE) {
          can->sendMessage(&msg);
          xSemaphoreGive(spiMutex);
        }
      }
    }
  }
}

void resetDataCanC(unsigned long timeout_ms) {
  static unsigned long now_ms = 0;
  static long long last_flushed = -1;
  if (millis() - now_ms > timeout_ms) {
    now_ms = millis();
    xSemaphoreTake(can0_stats.mutex, portMAX_DELAY);
    long long msg_time = can0_stats.last_msg_time;
    xSemaphoreGive(can0_stats.mutex);
    const bool offline = (esp_timer_get_time() - msg_time) > (long long)timeout_ms * 1000LL || msg_time == 0;
    if (offline && msg_time != last_flushed) {
      xSemaphoreTake(canMutex, portMAX_DELAY);
      memset(&ZGW_248h, 0, sizeof(ZGW_248h));           // ECU: ZGW, NAME: ZGW_248h, ID: 0x0248, MSG COUNT: 31
      memset(&FS_340h, 0, sizeof(FS_340h));             // ECU: LF_ABC, NAME: FS_340h, ID: 0x0340, MSG COUNT: 16
      memset(&EZS_240h, 0, sizeof(EZS_240h));           // ECU: EZS, NAME: EZS_240h, ID: 0x0240, MSG COUNT: 31
      memset(&MS_608h, 0, sizeof(MS_608h));             // ECU: MS, NAME: MS_608h, ID: 0x0608, MSG COUNT: 13
      xSemaphoreGive(canMutex);
      last_flushed = msg_time;
    }
  }
}

void resetDataCanB(unsigned long timeout_ms) {
  static unsigned long now_ms = 0;
  static long long last_flushed = -1;
  if (millis() - now_ms > timeout_ms) {
    now_ms = millis();
    xSemaphoreTake(can1_stats.mutex, portMAX_DELAY);
    long long msg_time = can1_stats.last_msg_time;
    xSemaphoreGive(can1_stats.mutex);
    const bool offline = (esp_timer_get_time() - msg_time) > (long long)timeout_ms * 1000LL || msg_time == 0;
    if (offline && msg_time != last_flushed) {
      xSemaphoreTake(canMutex, portMAX_DELAY);
      memset(&KOMBI_A5, 0, sizeof(KOMBI_A5));           // ECU: KOMBI, NAME: KOMBI_A5, ID: 0x01CA, MSG COUNT: 25
      memset(&UBF_A1, 0, sizeof(UBF_A1));               // ECU: UBF, NAME: UBF_A1, ID: 0x001A, MSG COUNT: 9
      memset(&TP_KOMBI_AGW1, 0, sizeof(TP_KOMBI_AGW1)); // ECU: KOMBI, NAME: TP_KOMBI_AGW1, ID: 0x01D0. MSG COUNT: 1
      xSemaphoreGive(canMutex);
      last_flushed = msg_time;
    }
  }
}