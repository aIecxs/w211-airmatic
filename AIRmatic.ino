/*
 * AIRmatic Air Suspension Level Control DIY Kit
 * 
 * Based on:
 * - MCP2515 Arduino Library (https://github.com/autowp/arduino-mcp2515)
 * - CAN Bus reverse engineering by @rnd-ash (https://github.com/rnd-ash/mb-w211-pc)
 * 
 * Licensed under the MIT License.
 */

#include <SPI.h>
#include <mcp2515.h>
#include <LittleFS.h>
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

int8_t offset_nv = 0; // mm front axle level custom offset
int8_t offset_nh = 0; // mm rear axle level custom offset
float factor = (float) duty * 2.0 / 100.0;

volatile unsigned long timer = 0; // millis()

volatile bool canInterruptFlag0 = false;
volatile bool canInterruptFlag1 = false;

TaskHandle_t canTask0;
TaskHandle_t canTask1;

// export CAN_message into bit field decoder
void exportMsg(unsigned int id, const uint8_t *msg, uint8_t len)
{
/*
  if (id != 0x0000) {
    Serial.write((uint8_t)(id >> 8));
    Serial.write((uint8_t)(id & 0xFF));
    Serial.write((uint8_t)len);
    Serial.write(msg, len);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
*/
  switch(id) {
    case CANID_0:
      if (len <= sizeof(EZS_240h)) {
        memcpy(&EZS_240h, msg, len); // ECU: EZS, NAME: EZS_240h, ID: 0x0240, MSG COUNT: 31
      } else {
        Serial.printf("WARNING: CANID 0x%04X: frame too long, struct size is %d bytes\r\n", id, sizeof(EZS_240h));
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      break;
    case CANID_1:
      if (len <= sizeof(FS_340h)) {
        memcpy(&FS_340h, msg, len); // ECU: LF_ABC, NAME: FS_340h, ID: 0x0340, MSG COUNT: 16
      } else {
        Serial.printf("WARNING: CANID 0x%04X: frame too long, struct size is %d bytes\r\n", id, sizeof(FS_340h));
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      break;
    case CANID_2:
      if (len <= sizeof(KOMBI_A5)) {
        memcpy(&KOMBI_A5, msg, len); // ECU: KOMBI, NAME: KOMBI_A5, ID: 0x01CA, MSG COUNT: 25
      } else {
        Serial.printf("WARNING: CANID 0x%04X: frame too long, struct size is %d bytes\r\n", id, sizeof(KOMBI_A5));
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      break;
    case CANID_3:
      if (len <= sizeof(UBF_A1)) {
        memcpy(&UBF_A1, msg, len); // ECU: UBF, NAME: UBF_A1, ID: 0x001A, MSG COUNT: 9
      } else {
        Serial.printf("WARNING: CANID 0x%04X: frame too long, struct size is %d bytes\r\n", id, sizeof(UBF_A1));
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      break;
  }
}

// not needed - only for debug
void _debug_EZS_240h_print(unsigned int delayMs)
{
  static unsigned long time = 0;
  if ( millis() - time > delayMs )
  {
    time = millis();
//    Serial.println("byte 1");
//    Serial.println("false");
    Serial.println("bitfield 2");
    Serial.print("KG_KL_AKT = "); Serial.print(EZS_240h.KG_KL_AKT, DEC); Serial.println(" Keyless Go terminal control active");
    Serial.print("KG_ALB_OK = "); Serial.print(EZS_240h.KG_ALB_OK, DEC); Serial.println(" Keyles Go occasion requirements met");
    Serial.print("LL_RLC = "); Serial.print(EZS_240h.LL_RLC, DEC); Serial.println(" Left Hand Drive/Right Hand Drive");
/*
    Serial.print("LL_RLC = ");
    if (EZS_240h.LL_RLC == EZS_240h_t::LL_RLC_c::NDEF) {
      Serial.print("NDEF Undefined,");
    } else if (EZS_240h.LL_RLC == EZS_240h_t::LL_RLC_c::LL) {
      Serial.print("LL Left hand drive,");
    } else if (EZS_240h.LL_RLC == EZS_240h_t::LL_RLC_c::RL) {
      Serial.print("RL Right hand drive,");
    } else if (EZS_240h.LL_RLC == EZS_240h_t::LL_RLC_c::SNV) {
      Serial.print("SNV Signal not available,");
    }
    Serial.println(" Left Hand Drive/Right Hand Drive");
*/
    Serial.print("RG_SCHALT = "); Serial.print(EZS_240h.RG_SCHALT, DEC); Serial.println(" reverse gear engaged (manual gearbox only)");
    Serial.print("BS_SL = "); Serial.print(EZS_240h.BS_SL, DEC); Serial.println(" Brake switch for shift lock");
    Serial.print("KL_15 = "); Serial.print(EZS_240h.KL_15, DEC); Serial.println(" Terminal 15");
    Serial.print("KL_50 = "); Serial.print(EZS_240h.KL_50, DEC); Serial.println(" Terminal 50");
//    Serial.println("byte 3");
//    Serial.println("false");
    Serial.println("bitfield 4");
    Serial.print("SAM_PAS = "); Serial.print(EZS_240h.SAM_PAS, DEC); Serial.println(" SAM/x passive, x = Bb (230), V (211), F (240)");
    Serial.print("BLS_A = "); Serial.print(EZS_240h.BLS_A, DEC); Serial.println(" SAM/x: brake light switch output EHB-ASG, x = B (230), V (211), F (240)");
    Serial.print("BN_SOCS = "); Serial.print(EZS_240h.BN_SOCS, DEC); Serial.println(" Vehicle electrical system warning: starter battery state of charge");
    Serial.print("ASG_SPORT_BET = "); Serial.print(EZS_240h.ASG_SPORT_BET, DEC); Serial.println(" ASG sport mode on/off actuated (ST2_LED_DL if ABC available)");
    Serial.print("VSTAT_A = "); Serial.print(EZS_240h.VSTAT_A, DEC); Serial.println(" SAM/x: v-signal from EHB-ASG, x = B (230), V (211), F ( 240");
    Serial.print("INF_RFE_SAM = "); Serial.print(EZS_240h.INF_RFE_SAM, DEC); Serial.println(" SAM/x: EHB-ASG in fallback level, x = B (230), V (211,164,251), F (240)");
    Serial.print("CRASH_CNF = "); Serial.print(EZS_240h.CRASH_CNF, DEC); Serial.println(" CRASH confirm bit");
    Serial.print("CRASH = "); Serial.print(EZS_240h.CRASH, DEC); Serial.println(" Crash signal from airbag SG");
    Serial.println("bitfield 5");
    Serial.print("BN_NTLF = "); Serial.print(EZS_240h.BN_NTLF, DEC); Serial.println(" Vehicle power supply emergency mode: Prio1 and Prio2 consumers off, second battery supports");
    Serial.print("ESP_BET = "); Serial.print(EZS_240h.ESP_BET, DEC); Serial.println(" ESP on/off actuated");
/*
    Serial.print("ESP_BET = ");
    if (EZS_240h.ESP_BET == EZS_240h_t::ESP_BET_c::NBET) {
      Serial.print("NBET Not operated (rocker and push push),");
    } else if (EZS_240h.ESP_BET == EZS_240h_t::ESP_BET_c::AUS_BET) {
      Serial.print("AUS_BET ESP off actuated (rocker), actuated (push push),");
    } else if (EZS_240h.ESP_BET == EZS_240h_t::ESP_BET_c::EIN_NDEF) {
      Serial.print("EIN_NDEF ESP on actuated (rocker), not defined (push push),");
    } else if (EZS_240h.ESP_BET == EZS_240h_t::ESP_BET_c::SNV) {
      Serial.print("SNV No signal (rocker and push push),");
    }
    Serial.println(" ESP on/off actuated");
*/
    Serial.print("HAS_KL = "); Serial.print(EZS_240h.HAS_KL, DEC); Serial.println(" Handbrake applied (indicator lamp)");
    Serial.print("KL_31B = "); Serial.print(EZS_240h.KL_31B, DEC); Serial.println(" Wiper out of park position");
    Serial.print("BLI_RE = "); Serial.print(EZS_240h.BLI_RE, DEC); Serial.println(" Turn signal right");
    Serial.print("BLI_LI = "); Serial.print(EZS_240h.BLI_LI, DEC); Serial.println(" Turn signal left");
    Serial.println("bitfield 6");
    Serial.print("ST2_BET = "); Serial.print(EZS_240h.ST2_BET, DEC); Serial.println(" LF/ABC 2-position switch actuated");
/*
    Serial.print("ST2_BET = ");
    if (EZS_240h.ST2_BET == EZS_240h_t::ST2_BET_c::NBET) {
      Serial.print("NBET Not operated (rocker and push push),");
    } else if (EZS_240h.ST2_BET == EZS_240h_t::ST2_BET_c::UNBET_NDEF) {
      Serial.print("UNBET_NDEF Bottom Actuated (Rocker), Undefined (Push Push),");
    } else if (EZS_240h.ST2_BET == EZS_240h_t::ST2_BET_c::OBBET_BET) {
      Serial.print("OBBET_BET Top Actuated (Rocker), Actuated (Push Push),");
    } else if (EZS_240h.ST2_BET == EZS_240h_t::ST2_BET_c::NDEF) {
      Serial.print("NDEF Undefined,");
    }
    Serial.println(" LF/ABC 2-position switch actuated");
*/
    Serial.print("ST3_BET = "); Serial.print(EZS_240h.ST3_BET, DEC); Serial.println(" LF/ABC 3-position switch actuated");
/*
    Serial.print("ST3_BET = ");
    if (EZS_240h.ST3_BET == EZS_240h_t::ST3_BET_c::NBET) {
      Serial.print("NBET Not operated (rocker and push push),");
    } else if (EZS_240h.ST3_BET == EZS_240h_t::ST3_BET_c::UNBET_NDEF) {
      Serial.print("UNBET_NDEF Bottom Actuated (Rocker), Undefined (Push Push),");
    } else if (EZS_240h.ST3_BET == EZS_240h_t::ST3_BET_c::OBBET_BET) {
      Serial.print("OBBET_BET Top Actuated (Rocker), Actuated (Push Push),");
    } else if (EZS_240h.ST3_BET == EZS_240h_t::ST3_BET_c::NDEF) {
      Serial.print("NDEF Undefined,");
    }
    Serial.println(" LF/ABC 3-position switch actuated");
*/
    Serial.print("ART_ABW_BET = "); Serial.print(EZS_240h.ART_ABW_BET, DEC); Serial.println(" ART distance warning on/off actuated");
/*
    Serial.print("ART_ABW_BET = ");
    if (EZS_240h.ART_ABW_BET == EZS_240h_t::ART_ABW_BET_c::NDEF_NBET) {
      Serial.print("NDEF_NBET not defined (rocker), not actuated (push push),");
    } else if (EZS_240h.ART_ABW_BET == EZS_240h_t::ART_ABW_BET_c::AUS_NDEF) {
      Serial.print("AUS_NDEF distance warning off (rocker), not defined (push push),");
    } else if (EZS_240h.ART_ABW_BET == EZS_240h_t::ART_ABW_BET_c::EIN_BET) {
      Serial.print("EIN_BET Distance warning on (rocker), actuated (push push),");
    } else if (EZS_240h.ART_ABW_BET == EZS_240h_t::ART_ABW_BET_c::SNV) {
      Serial.print("SNV No signal (rocker and push push),");
    }
    Serial.println(" ART distance warning on/off actuated");
*/
    Serial.print("ABL_EIN = "); Serial.print(EZS_240h.ABL_EIN, DEC); Serial.println(" Turn on low beam");
    Serial.print("KL54_RM = "); Serial.print(EZS_240h.KL54_RM, DEC); Serial.println(" Terminal 54 hardware active");
    Serial.println("byte 7");
    Serial.print("ART_ABSTAND = "); Serial.print(EZS_240h.ART_ABSTAND, DEC); Serial.println(" distance factor");
    Serial.println("bitfield 8");
    Serial.print("ART_VH = "); Serial.print(EZS_240h.ART_VH, DEC); Serial.println(" ART available");
    Serial.print("GBL_AUS = "); Serial.print(EZS_240h.GBL_AUS, DEC); Serial.println(" E-suction fan: Basic ventilation off");
    Serial.print("FZGVERSN = "); Serial.print(EZS_240h.FZGVERSN, DEC); Serial.println(" Series-dependent vehicle version (only 220/215/230)");
/*
    Serial.print("FZGVERSN = ");
    if (EZS_240h.FZGVERSN == EZS_240h_t::FZGVERSN_c::START) {
      Serial.print("START Status at market launch of the respective series,");
    } else if (EZS_240h.FZGVERSN == EZS_240h_t::FZGVERSN_c::V1) {
      Serial.print("V1 BR 220: AJ 99/X, C215: AJ 01/1, R230: AJ 02/1,");
    } else if (EZS_240h.FZGVERSN == EZS_240h_t::FZGVERSN_c::V2) {
      Serial.print("V2 BR 220: AJ 01/1, C215: AJ 02/X, R230: AJ 03/X,");
    } else if (EZS_240h.FZGVERSN == EZS_240h_t::FZGVERSN_c::V3) {
      Serial.print("V3 BR 220: ÄJ 02/X, C215: ÄJ 03/X, R230: not defined,");
    } else if (EZS_240h.FZGVERSN == EZS_240h_t::FZGVERSN_c::V4) {
      Serial.print("V4 BR 220: prohibited, C215/R230: not defined,");
    } else if (EZS_240h.FZGVERSN == EZS_240h_t::FZGVERSN_c::V5) {
      Serial.print("V5 BR 220: prohibited, C215/R230: not defined,");
    } else if (EZS_240h.FZGVERSN == EZS_240h_t::FZGVERSN_c::V6) {
      Serial.print("V6 BR 220: ÄJ 03/X, C215,/R230: not defined,");
    } else if (EZS_240h.FZGVERSN == EZS_240h_t::FZGVERSN_c::V7) {
      Serial.print("V7 BR 220/ C215,/R230: not defined,");
    }
    Serial.println(" Series-dependent vehicle version (only 220/215/230)");
*/
    Serial.print("LDC = "); Serial.print(EZS_240h.LDC, DEC); Serial.println(" country code");
/*
    Serial.print("LDC = ");
    if (EZS_240h.LDC == EZS_240h_t::LDC_c::RDW) {
      Serial.print("RDW Rest of the world,");
    } else if (EZS_240h.LDC == EZS_240h_t::LDC_c::USA_CAN) {
      Serial.print("USA_CAN USA/Canada,");
    } else if (EZS_240h.LDC == EZS_240h_t::LDC_c::NDEF) {
      Serial.print("NDEF Undefined,");
    } else if (EZS_240h.LDC == EZS_240h_t::LDC_c::SNV) {
      Serial.print("SNV Signal not available,");
    }
    Serial.println(" country code");
*/
  }
}

// not needed - only for debug
void _debug_FS_340h_print(unsigned int delayMs)
{
  static unsigned long time = 0;
  if ( millis() - time > delayMs )
  {
    time = millis();
    Serial.println("bitfield 1");
    Serial.print("NEDG = "); Serial.print(FS_340h.NEDG, DEC); Serial.println(" Level calibration performed");
    Serial.print("M2 = "); Serial.print(FS_340h.M2, DEC); Serial.println(" Message 2: \"Level selection deleted\"");
    Serial.print("M1 = "); Serial.print(FS_340h.M1, DEC); Serial.println(" Message 1: \"Vehicle lifts\", BR164/251: \"Highway->Offroad\", BR164 Offroad: \"Highway->Offroad1\"");
    Serial.print("FM4 = "); Serial.print(FS_340h.FM4, DEC); Serial.println(" Error 4: \"Park vehicle\"");
    Serial.print("FM3 = "); Serial.print(FS_340h.FM3, DEC); Serial.println(" Error 3: \"Visit workshop\"");
    Serial.print("FM2 = "); Serial.print(FS_340h.FM2, DEC); Serial.println(" Error 2: \"wait a moment\" (LF)/ \"steering oil\" (only ABC)");
    Serial.print("FM1 = "); Serial.print(FS_340h.FM1, DEC); Serial.println(" Error 1: \"Stop car too low\"");
    Serial.println("bitfield 2");
    Serial.print("BELAD = "); Serial.print(FS_340h.BELAD, DEC); Serial.println(" loading");
/*
    Serial.print("BELAD = ");
    if (FS_340h.BELAD == FS_340h_t::BELAD_c::LEER) {
      Serial.print("LEER Unloaded,");
    } else if (FS_340h.BELAD == FS_340h_t::BELAD_c::HALB) {
      Serial.print("HALB Half loaded,");
    } else if (FS_340h.BELAD == FS_340h_t::BELAD_c::VOLL) {
      Serial.print("VOLL Fully loaded,");
    } else if (FS_340h.BELAD == FS_340h_t::BELAD_c::SNV) {
      Serial.print("SNV Load not recognized,");
    }
    Serial.println(" loading");
*/
    Serial.print("ST2_LED_DL = "); Serial.print(FS_340h.ST2_LED_DL, DEC); Serial.println(" LED 2-stage switch steady light");
    Serial.print("ST3_LEDR_DL = "); Serial.print(FS_340h.ST3_LEDR_DL, DEC); Serial.println(" Right LED 3-position switch steady light (164/251 top LED)");
    Serial.print("ST3_LEDL_DL = "); Serial.print(FS_340h.ST3_LEDL_DL, DEC); Serial.println(" Left LED 3-position switch steady light (164/251 lower LED)");
    Serial.println("byte 3, 4, 5, 6");
    Serial.print("FZGN_VL = "); Serial.print(FS_340h.FZGN_VL, DEC); Serial.println(" Vehicle level, front left");
    Serial.print("FZGN_VR = "); Serial.print(FS_340h.FZGN_VR, DEC); Serial.println(" Vehicle level, front right");
    Serial.print("FZGN_HL = "); Serial.print(FS_340h.FZGN_HL, DEC); Serial.println(" Vehicle level, rear left");
    Serial.print("FZGN_HR = "); Serial.print(FS_340h.FZGN_HR, DEC); Serial.println(" Vehicle level, rear right");
//    Serial.println("byte 7");
//    Serial.println("false");
    Serial.println("bitfield 8");
    Serial.print("FS_ID = "); Serial.print(FS_340h.FS_ID, DEC); Serial.println(" Suspension control identification");
/*
    Serial.print("FS_ID = ");
    if (FS_340h.FS_ID == FS_340h_t::FS_ID_c::LF) {
      Serial.print("LF Air suspension/ LF (BR164/251 NR without ADS),");
    } else if (FS_340h.FS_ID == FS_340h_t::FS_ID_c::SLF) {
      Serial.print("SLF Semi-active air suspension, SLF (BR164/251 NR+ADS),");
    } else if (FS_340h.FS_ID == FS_340h_t::FS_ID_c::EHNR) {
      Serial.print("EHNR Electronic rear axle level control,");
    } else if (FS_340h.FS_ID == FS_340h_t::FS_ID_c::ABC) {
      Serial.print("ABC Active Body Control 1,");
    }
    Serial.println(" Suspension control identification");
*/
  }
}

// not needed - only for debug
void _debug_KOMBI_A5_print(unsigned int delayMs)
{
  static unsigned long time = 0;
  if ( millis() - time > delayMs )
  {
    time = millis();
    Serial.println("bitfield 1");
    Serial.print("KI_STAT = "); Serial.print(KOMBI_A5.KI_STAT, DEC); Serial.println(" Status Kombi"); 
/*
    Serial.print("KI_STAT = ");
    if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::Neutral) {
      Serial.print("2 Neutral,");
    } else if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::AUDIO) {
      Serial.print("3 AUDIO Audio,");
    } else if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::NAVI) {
      Serial.print("4 NAVI Navigation,");
    } else if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::TEL) {
      Serial.print("5 TEL Telefon,");
    } else if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::NEU_SER) {
      Serial.print("6 NEU_SER Neue Services,");
    } else if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::SPR_FNK_DLG_CLO) {
      Serial.print("19 SPR_FNK_DLG_CLO Sprachfunk-KI Dlg geschl,");
    } else if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::DAT_FNK_DLG_CLO) {
      Serial.print("20 DAT_FNK_DLG_CLO Datenfunk-KI Dlg geschl,");
    } else if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::SPR_FNK_DLG_OPN) {
      Serial.print("21 SPR_FNK_DLG_OPN Sprachfunk-KI Dlg geöffnet,");
    } else if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::DAT_FNK_DLG_OPN) {
      Serial.print("22 DAT_FNK_DLG_OPN Datenfunk-KI Dlg geöffnet,");
    } else if (KOMBI_A5.KI_STAT == KOMBI_A5_t::KI_STAT_c::SNV) {
      Serial.print("255 SNV Signal nicht vorhanden,");
    }
    Serial.println(" Status Kombi");
*/
    Serial.println("bitfield 2");
    Serial.print("BUTTON_4_2 = "); Serial.print(KOMBI_A5.BUTTON_4_2, DEC); Serial.println(" Telefon End");
    Serial.print("BUTTON_4_1 = "); Serial.print(KOMBI_A5.BUTTON_4_1, DEC); Serial.println(" Telefon Send");
    Serial.print("BUTTON_3_2 = "); Serial.print(KOMBI_A5.BUTTON_3_2, DEC); Serial.println(" Taste \"-\"");
    Serial.print("BUTTON_3_1 = "); Serial.print(KOMBI_A5.BUTTON_3_1, DEC); Serial.println(" Taste \"+\"");
    Serial.print("BUTTON_2_2 = "); Serial.print(KOMBI_A5.BUTTON_2_2, DEC); Serial.println(" Reserve");
    Serial.print("BUTTON_2_1 = "); Serial.print(KOMBI_A5.BUTTON_2_1, DEC); Serial.println(" Reserve");
    Serial.print("BUTTON_1_2 = "); Serial.print(KOMBI_A5.BUTTON_1_2, DEC); Serial.println(" Vorheriges Display");
    Serial.print("BUTTON_1_1 = "); Serial.print(KOMBI_A5.BUTTON_1_1, DEC); Serial.println(" Nächstes Display");
    Serial.println("bitfield 3");
    Serial.print("BUTTON_8_2 = "); Serial.print(KOMBI_A5.BUTTON_8_2, DEC); Serial.println(" Reserve");
    Serial.print("BUTTON_8_1 = "); Serial.print(KOMBI_A5.BUTTON_8_1, DEC); Serial.println(" Reserve");
    Serial.print("BUTTON_7_2 = "); Serial.print(KOMBI_A5.BUTTON_7_2, DEC); Serial.println(" Reserve");
    Serial.print("BUTTON_7_1 = "); Serial.print(KOMBI_A5.BUTTON_7_1, DEC); Serial.println(" Reserve");
    Serial.print("BUTTON_6_2 = "); Serial.print(KOMBI_A5.BUTTON_6_2, DEC); Serial.println(" Reserve");
    Serial.print("BUTTON_6_1 = "); Serial.print(KOMBI_A5.BUTTON_6_1, DEC); Serial.println(" Reserve");
    Serial.print("BUTTON_5_2 = "); Serial.print(KOMBI_A5.BUTTON_5_2, DEC); Serial.println(" Reserve");
    Serial.print("BUTTON_5_1 = "); Serial.print(KOMBI_A5.BUTTON_5_1, DEC); Serial.println(" Reserve");
    Serial.println("bitfield 4");
    Serial.print("PTT_4_2 = "); Serial.print(KOMBI_A5.PTT_4_2, DEC); Serial.println(" Reserve");
    Serial.print("PTT_4_1 = "); Serial.print(KOMBI_A5.PTT_4_1, DEC); Serial.println(" Reserve");
    Serial.print("PTT_3_2 = "); Serial.print(KOMBI_A5.PTT_3_2, DEC); Serial.println(" Reserve");
    Serial.print("PTT_3_1 = "); Serial.print(KOMBI_A5.PTT_3_1, DEC); Serial.println(" Reserve");
    Serial.print("PTT_2_2 = "); Serial.print(KOMBI_A5.PTT_2_2, DEC); Serial.println(" Reserve");
    Serial.print("PTT_2_1 = "); Serial.print(KOMBI_A5.PTT_2_1, DEC); Serial.println(" Reserve");
    Serial.print("PTT_1_2 = "); Serial.print(KOMBI_A5.PTT_1_2, DEC); Serial.println(" Linguatronic deaktivieren");
    Serial.print("PTT_1_1 = "); Serial.print(KOMBI_A5.PTT_1_1, DEC); Serial.println(" Linguatronic aktivieren");
  }
}

// not needed - only for debug
void _debug_UBF_A1_print(unsigned int delayMs)
{
  static unsigned long time = 0;
  if ( millis() - time > delayMs )
  {
    time = millis();
    Serial.println("bitfield 1");
    Serial.print("ART_ABW_BET = "); Serial.print(UBF_A1.ART_ABW_BET, DEC); Serial.println(" ART-Abstandswarnung ein/aus betätigt");
    Serial.print("FU_FRSP_BET = "); Serial.print(UBF_A1.FU_FRSP_BET, DEC); Serial.println(" Taster Funkaufschaltung betätigt");
/*
    Serial.print("ART_ABW_BET = ");
    if (UBF_A1.ART_ABW_BET == UBF_A1_t::ART_ABW_BET_c::NDEF_NBET) {
      Serial.print("0 NDEF_NBET nicht definiert (Wippe), Nicht betätigt (Push Push),");
    } else if (UBF_A1.ART_ABW_BET == UBF_A1_t::ART_ABW_BET_c::AUS_NDEF) {
      Serial.print("1 AUS_NDEF Abstandswarnung aus (Wippe), nicht definiert (Push Push),");
    } else if (UBF_A1.ART_ABW_BET == UBF_A1_t::ART_ABW_BET_c::EIN_BET) {
      Serial.print("2 EIN_BET Abstandswarnung ein (Wippe), Betätigt (Push Push),");
    } else if (UBF_A1.ART_ABW_BET == UBF_A1_t::ART_ABW_BET_c::SNV) {
      Serial.print("3 SNV Signal nicht vorhanden (Wippe und Push Push),");
    }
    Serial.println(" ART-Abstandswarnung ein/aus betätigt");
*/
    Serial.println("byte 2");
    Serial.print("ART_ABSTAND = "); Serial.print(UBF_A1.ART_ABSTAND, DEC); Serial.println(" Abstandsfaktor");
    Serial.println("bitfield 3");
    Serial.print("PTS_BET = "); Serial.print(UBF_A1.PTS_BET, DEC); Serial.println(" Taster Parktronic betätigt");
    Serial.print("BH_FUNK_BET = "); Serial.print(UBF_A1.BH_FUNK_BET, DEC); Serial.println(" Taster Behördenfunk betätigt");
    Serial.print("ST2_BET = "); Serial.print(UBF_A1.ST2_BET, DEC); Serial.println(" 2-stufiger Taster betätigt");
    Serial.print("ST3_BET = "); Serial.print(UBF_A1.ST3_BET, DEC); Serial.println(" 3-stufiger Taster betätigt");
    Serial.println("bitfield 4");
    Serial.print("STHL_BET = "); Serial.print(UBF_A1.STHL_BET, DEC); Serial.println(" Schalter Standheizung betätigt");
    Serial.print("LED_STH_DEF = "); Serial.print(UBF_A1.LED_STH_DEF, DEC); Serial.println(" LEDs für Standheizung defekt");
  }
}

void SerialPrintWarn(String text, unsigned int value, unsigned int delayMs) {
  static unsigned long time = millis();
  if ( millis() - time > delayMs ) {
    time = millis();
    char buf[7];
    sprintf(buf, "0x%04X", value);
    Serial.print(text);
    Serial.println(buf);
  }
}

void blink(gpio_num_t pin, unsigned int ontime) {
  bool lastState = digitalRead(pin);
  digitalWrite(pin, !lastState);
  vTaskDelay(pdMS_TO_TICKS(ontime));
  digitalWrite(pin, lastState);
}

// debounce key input
uint8_t get_keyevent(bool button, uint8_t index) {
  uint32_t now = millis();
  static uint32_t pressTime[8] = {0};   // 0 = not pressed, nonzero = press timestamp
  const uint32_t autoResetDelay = 1000; // max press time (ms)
  if (button) {
    if (pressTime[index] != 0) {
      if (now - pressTime[index] > autoResetDelay) {
        pressTime[index] = 0;
        return 2;  // timeout (auto-reset)
      } else {
        return 1;
      }
    } else {
      pressTime[index] = now;
      return 1;  // rising edge detected
    }
  } else {
    pressTime[index] = 0;
    return 0; // not pressed
  }
}

// Interrupt based CanRx
void IRAM_ATTR onCanInterrupt0() {
  canInterruptFlag0 = true;
}
void IRAM_ATTR onCanInterrupt1() {
  canInterruptFlag1 = true;
}

// power down no CAN traffic
void go_to_sleep(unsigned int timeout) {
  if ( millis() - timer > timeout ) {
    timer = millis();
    Serial.println("Can1: timeout ... no traffic");
    // put TJA1055 into go-to-sleep
    digitalWrite(STB, LOW);
    delay(1);
    digitalWrite(EN, LOW);
    delay(100);
  }
}

// keep awake CAN traffic
void awake(unsigned int delayMs) {
  static unsigned long time = millis();
  if ( millis() - time > delayMs ) {
    time = millis();
    timer = time;
    // put TJA1055 into Normal Mode
    digitalWrite(STB, HIGH);
    digitalWrite(EN, HIGH);
  }
}

void setup() {
  // create a task that will be executed along the loop() function, with priority 3 and executed on core 0
  xTaskCreatePinnedToCore(
    canEvent0,     // Task function
    "Can0 Events", // name of task
    4096,          // Stack size of task
    NULL,          // parameter of the task
    3,             // priority of the task
    &canTask0,     // Task handle to keep track of created task
    0);            // pin task to core 0
  
  delay(100);

  // create a task that will be executed along the loop() function, with priority 2 and executed on core 1
  xTaskCreatePinnedToCore(
    canEvent1,     // Task function
    "Can1 Events", // name of task
    4096,          // Stack size of task
    NULL,          // parameter of the task
    2,             // priority of the task
    &canTask1,     // Task handle to keep track of created task
    1);            // pin task to core 1

  Serial.begin(115200, SERIAL_8N1, RX0, TX0);
  SPI.begin(SCK, MISO, MOSI);

  /* SPI flash file system
  |--------------|-------|---------------|--|--|--|--|--|
  ^              ^       ^               ^     ^
  Sketch    OTA update   File system   EEPROM  WiFi config (SDK) */
  LittleFS.begin();

  pinMode(LED_BUILTIN, OUTPUT);

  // MCP2515 Interrupts
  pinMode(INT0, INPUT);
  pinMode(INT1, INPUT);
  attachInterrupt(digitalPinToInterrupt(INT0), onCanInterrupt0, FALLING);
  attachInterrupt(digitalPinToInterrupt(INT1), onCanInterrupt1, FALLING);
  
  // Normal Mode TJA1055
  pinMode(EN, OUTPUT);
  pinMode(STB, OUTPUT);
  digitalWrite(EN, HIGH);
  digitalWrite(STB, HIGH);

  // Watchdog output
  pinMode(WO, OUTPUT);
  
  // DAC offset voltage generator 4 kHz
  ledcAttach(PWM1, freq, res);
  ledcAttach(PWM2, freq, res);
  ledcAttach(PWM3, freq, res);
  ledcAttach(PWM4, freq, res);

  // DAC offset: Vref on
  pinMode(ON, OUTPUT);
  digitalWrite(ON, HIGH);

  Can0.reset();
  Can1.reset();
  delay(1);

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
}

void loop() {
  uint8_t pressedA = 0;
  uint8_t pressedB = 0;
/*
  // DEBUG
  exportMsg(CANID_0, (const uint8_t[]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8);
  exportMsg(CANID_1, (const uint8_t[]){0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, 8);
  exportMsg(CANID_2, (const uint8_t[]){0xFF, 0xFF, 0xFF, 0xFF}, 4);
  exportMsg(CANID_3, (const uint8_t[]){0xFF, 0xFF, 0xFF, 0xFF}, 4);
  _debug_EZS_240h_print(1000);
  _debug_FS_340h_print(1000);
  _debug_KOMBI_A5_print(1000);
  _debug_UBF_A1_print(1000);
*/
/*
  // main loop
  if (FS_340h.FS_ID == FS_340h_t::FS_ID_c::EHNR) {
    SerialPrintWarn("EHNR mode", 2, 1000);

    // check the KOMBI_A9 IPS Mode (Comfort/Sport)


  } else if (FS_340h.FS_ID == FS_340h_t::FS_ID_c::SLF) {
    SerialPrintWarn("AIRmatic mode", 1, 1000);

    // check the AIRmatic mode (Offroad/Comfort/Sport1/Sport2)
    // read offsets from table
      // on table: apply offsets
      offset_nv = 0;                   // mm front axle level custom offset
      offset_nh = offset_nv;             // mm rear axle level custom offset
*/
    // check display is in phone mode
    if (KOMBI_A5.KI_STAT == 5) {
      // get keyevents
      // FRONT + HIGHER
      pressedA = get_keyevent(KOMBI_A5.BUTTON_1_1, 0);
      pressedB = get_keyevent(KOMBI_A5.BUTTON_3_1, 4);
      if (pressedA && pressedB) {
        if (pressedA == 2) {
          KOMBI_A5.BUTTON_1_1 = 0; // timeout (auto-reset)
        }
        if (pressedB == 2) {
          KOMBI_A5.BUTTON_3_1 = 0; // timeout (auto-reset)
        }
        offset_nv += 5; // increase
        Serial.print("BUTTON_1_1 = "); Serial.print(KOMBI_A5.BUTTON_1_1, DEC); Serial.println(" Nächstes Display");   // FRONT
        Serial.print("BUTTON_3_1 = "); Serial.print(KOMBI_A5.BUTTON_3_1, DEC); Serial.println(" Taste \"+\"");  // HIGHER
        Serial.print("offset_nv = "); Serial.print(offset_nv, DEC); Serial.println(" mm front axle level custom offset");
        blink(LED_BUILTIN, 100);
      }
      // FRONT + LOWER
      pressedA = get_keyevent(KOMBI_A5.BUTTON_1_1, 0);
      pressedB = get_keyevent(KOMBI_A5.BUTTON_3_2, 5);
      if (pressedA && pressedB) {
        if (pressedA == 2) {
          KOMBI_A5.BUTTON_1_1 = 0; // timeout (auto-reset)
        }
        if (pressedB == 2) {
          KOMBI_A5.BUTTON_3_2 = 0; // timeout (auto-reset)
        }
        offset_nv -= 5; // decrease
        Serial.print("BUTTON_1_1 = "); Serial.print(KOMBI_A5.BUTTON_1_1, DEC); Serial.println(" Nächstes Display");   // FRONT
        Serial.print("BUTTON_3_2 = "); Serial.print(KOMBI_A5.BUTTON_3_2, DEC); Serial.println(" Taste \"-\"");  // LOWER
        Serial.print("offset_nv = "); Serial.print(offset_nv, DEC); Serial.println(" mm front axle level custom offset");
        blink(LED_BUILTIN, 100);
      }
      // REAR + HIGHER
      pressedA = get_keyevent(KOMBI_A5.BUTTON_1_2, 1);
      pressedB = get_keyevent(KOMBI_A5.BUTTON_3_1, 4);
      if (pressedA && pressedB) {
        if (pressedA == 2) {
          KOMBI_A5.BUTTON_1_2 = 0; // timeout (auto-reset)
        }
        if (pressedB == 2) {
          KOMBI_A5.BUTTON_3_1 = 0; // timeout (auto-reset)
        }
        offset_nh += 5; // increase
        Serial.print("BUTTON_1_2 = "); Serial.print(KOMBI_A5.BUTTON_1_2, DEC); Serial.println(" Vorheriges Display"); // REAR
        Serial.print("BUTTON_3_1 = "); Serial.print(KOMBI_A5.BUTTON_3_1, DEC); Serial.println(" Taste \"+\"");  // HIGHER
        Serial.print("offset_nh = "); Serial.print(offset_nh, DEC); Serial.println(" mm rear axle level custom offset");
        blink(LED_BUILTIN, 100);
      }
      // REAR + LOWER
      pressedA = get_keyevent(KOMBI_A5.BUTTON_1_2, 1);
      pressedB = get_keyevent(KOMBI_A5.BUTTON_3_2, 5);
      if (pressedA && pressedB) {
        if (pressedA == 2) {
          KOMBI_A5.BUTTON_1_2 = 0; // timeout (auto-reset)
        }
        if (pressedB == 2) {
          KOMBI_A5.BUTTON_3_2 = 0; // timeout (auto-reset)
        }
        offset_nh -= 5; // decrease
        Serial.print("BUTTON_1_2 = "); Serial.print(KOMBI_A5.BUTTON_1_2, DEC); Serial.println(" Vorheriges Display"); // REAR
        Serial.print("BUTTON_3_2 = "); Serial.print(KOMBI_A5.BUTTON_3_2, DEC); Serial.println(" Taste \"-\"");  // LOWER
        Serial.print("offset_nh = "); Serial.print(offset_nh, DEC); Serial.println(" mm rear axle level custom offset");
        blink(LED_BUILTIN, 100);
      }

      // on confirm: write offsets to table
      // FRONT + SAVE
      pressedA = get_keyevent(KOMBI_A5.BUTTON_1_1, 0);
      pressedB = get_keyevent(KOMBI_A5.BUTTON_4_1, 6);
      if (pressedA && pressedB) {
        if (pressedA == 2) {
          KOMBI_A5.BUTTON_1_1 = 0; // timeout (auto-reset)
        }
        if (pressedB == 2) {
          KOMBI_A5.BUTTON_4_1 = 0; // timeout (auto-reset)
        }
        Serial.print("BUTTON_1_1 = "); Serial.print(KOMBI_A5.BUTTON_1_1, DEC); Serial.println(" Nächstes Display");   // FRONT
        Serial.print("BUTTON_4_1 = "); Serial.print(KOMBI_A5.BUTTON_4_1, DEC); Serial.println(" Telefon Send"); // SAVE
        Serial.print("offset_nv = "); Serial.print(offset_nv, DEC); Serial.println(" mm front axle level custom offset");
        // on confirm: write offsets to table
        blink(LED_BUILTIN, 100);
      }
      // REAR + SAVE
      pressedA = get_keyevent(KOMBI_A5.BUTTON_1_2, 1);
      pressedB = get_keyevent(KOMBI_A5.BUTTON_4_1, 6);
      if (pressedA && pressedB) {
        if (pressedA == 2) {
          KOMBI_A5.BUTTON_1_2 = 0; // timeout (auto-reset)
        }
        if (pressedB == 2) {
          KOMBI_A5.BUTTON_4_1 = 0; // timeout (auto-reset)
        }
        Serial.print("BUTTON_1_2 = "); Serial.print(KOMBI_A5.BUTTON_1_2, DEC); Serial.println(" Vorheriges Display"); // REAR
        Serial.print("BUTTON_4_1 = "); Serial.print(KOMBI_A5.BUTTON_4_1, DEC); Serial.println(" Telefon Send"); // SAVE
        Serial.print("offset_nh = "); Serial.print(offset_nh, DEC); Serial.println(" mm rear axle level custom offset");
        // on confirm: write offsets to table
        blink(LED_BUILTIN, 100);
      }
      // FRONT + CANCEL
      pressedA = get_keyevent(KOMBI_A5.BUTTON_1_1, 0);
      pressedB = get_keyevent(KOMBI_A5.BUTTON_4_2, 7);
      if (pressedA && pressedB) {
        if (pressedA == 2) {
          KOMBI_A5.BUTTON_1_1 = 0; // timeout (auto-reset)
        }
        if (pressedB == 2) {
          KOMBI_A5.BUTTON_4_2 = 0; // timeout (auto-reset)
        }
        offset_nv = 0;
        Serial.print("BUTTON_1_1 = "); Serial.print(KOMBI_A5.BUTTON_1_1, DEC); Serial.println(" Nächstes Display");   // FRONT
        Serial.print("BUTTON_4_2 = "); Serial.print(KOMBI_A5.BUTTON_4_2, DEC); Serial.println(" Telefon End");  // CANCEL
        Serial.print("offset_nv = "); Serial.print(offset_nv, DEC); Serial.println(" mm front axle level custom offset");
        // on confirm: write offsets to table
        blink(LED_BUILTIN, 100);
      }
      // REAR + CANCEL
      pressedA = get_keyevent(KOMBI_A5.BUTTON_1_2, 1);
      pressedB = get_keyevent(KOMBI_A5.BUTTON_4_2, 7);
      if (pressedA && pressedB) {
        if (pressedA == 2) {
          KOMBI_A5.BUTTON_1_2 = 0; // timeout (auto-reset)
        }
        if (pressedB == 2) {
          KOMBI_A5.BUTTON_4_2 = 0; // timeout (auto-reset)
        }
        offset_nh = 0;
        Serial.print("BUTTON_1_2 = "); Serial.print(KOMBI_A5.BUTTON_1_2, DEC); Serial.println(" Vorheriges Display"); // REAR
        Serial.print("BUTTON_4_2 = "); Serial.print(KOMBI_A5.BUTTON_4_2, DEC); Serial.println(" Telefon End");  // CANCEL
        Serial.print("offset_nh = "); Serial.print(offset_nh, DEC); Serial.println(" mm rear axle level custom offset");
        // on confirm: write offsets to table
        blink(LED_BUILTIN, 100);
      }
    }
/*
  } else {
    SerialPrintWarn("Can0: timeout ... no traffic for CAN-ID: ", CANID_1, 10000);
    // turn off
  }
*/
  // DAC offset voltage output
  ledcWrite(PWM1, duty + offset_nv * factor); // NVLS1
  ledcWrite(PWM2, duty - offset_nv * factor); // NVRS1 / inverted, requires negative offset
  ledcWrite(PWM3, duty + offset_nh * factor); // NHRS1
//  ledcWrite(PWM4, duty + offset_nh * factor); // NHLS1

  // todo: set custom level offset from steering wheel buttons

  // for safety purposes - do not change
  offset_nv = offset_nv < -MAX_OFF ? -MAX_OFF : offset_nv; // max suspension lowering
  offset_nv = offset_nv > MAX_OFF ? MAX_OFF : offset_nv;   // max suspension height

  offset_nh = offset_nh < -MAX_OFF ? -MAX_OFF : offset_nh; // max suspension lowering
  offset_nh = offset_nh > MAX_OFF ? MAX_OFF : offset_nh;   // max suspension height

  // put TJA1055 into go-to-sleep / TJA1055 does the rest and will switch off TLE4271 automatically
  go_to_sleep(10000); // 10 sec

  // Watchdog output pulse
  digitalWrite(WO, !digitalRead(WO));
  delay(100);
}