#ifndef W211_CAN_C_H
#define W211_CAN_C_H


// ECU: EZS, NAME: EZS_240h, ID: 0x0240, MSG COUNT: 31
typedef struct EZS_240h_t {
// byte 0
  uint8_t                         : 8; // Padding, OFFSET: 0, LEN: 8
// byte 1
  bool KL_50                      : 1; // DESC: KL_50 - Terminal 50, OFFSET: 15, LEN: 1
  bool KL_15                      : 1; // DESC: KL_15 - Terminal 15, OFFSET: 14, LEN: 1
  bool BS_SL                      : 1; // DESC: BS_SL - Brake switch for shift lock, OFFSET: 13, LEN: 1
  bool RG_SCHALT                  : 1; // DESC: RG_SCHALT - reverse gear engaged (manual gearbox only), OFFSET: 12, LEN: 1
  uint8_t LL_RLC                  : 2; // DESC: LL_RLC - Left Hand Drive/Right Hand Drive, OFFSET: 10, LEN: 2
/*
  enum class LL_RLC_c : uint8_t {      // DESC: LL_RLC - Left Hand Drive/Right Hand Drive, OFFSET: 10, LEN: 2
    NDEF                          = 0, // enum: 0 - Undefined / NDEF
    LL                            = 1, // enum: 1 - Left hand drive / LL
    RL                            = 2, // enum: 2 - Right hand drive / RL
    SNV                           = 3, // enum: 3 - Signal not available / SNV
  } : 2; LL_RLC_c LL_RLC;
*/
  bool KG_ALB_OK                  : 1; // DESC: KG_ALB_OK - Keyles Go occasion requirements met, OFFSET: 9, LEN: 1
  bool KG_KL_AKT                  : 1; // DESC: KG_KL_AKT - Keyless Go terminal control active, OFFSET: 8, LEN: 1
// byte 2
  uint8_t                         : 8; // Padding, OFFSET: 16, LEN: 8
// byte 3
  bool CRASH                      : 1; // DESC: CRASH - Crash signal from airbag SG, OFFSET: 31, LEN: 1
  bool CRASH_CNF                  : 1; // DESC: CRASH_CNF - CRASH confirm bit, OFFSET: 30, LEN: 1
  bool INF_RFE_SAM                : 1; // DESC: INF_RFE_SAM - SAM/x: EHB-ASG in fallback level, x = B (230), V (211,164,251), F (240), OFFSET: 29, LEN: 1
  bool VSTAT_A                    : 1; // DESC: VSTAT_A - SAM/x: v-signal from EHB-ASG, x = B (230), V (211), F ( 240), OFFSET: 28, LEN: 1
  bool ASG_SPORT_BET              : 1; // DESC: ASG_SPORT_BET - ASG sport mode on/off actuated (ST2_LED_DL if ABC available), OFFSET: 27, LEN: 1
  bool BN_SOCS                    : 1; // DESC: BN_SOCS - Vehicle electrical system warning: starter battery state of charge, OFFSET: 26, LEN: 1
  bool BLS_A                      : 1; // DESC: BLS_A - SAM/x: brake light switch output EHB-ASG, x = B (230), V (211), F (240), OFFSET: 25, LEN: 1
  bool SAM_PAS                    : 1; // DESC: SAM_PAS - SAM/x passive, x = Bb (230), V (211), F (240), OFFSET: 24, LEN: 1
// byte 4
  bool BLI_LI                     : 1; // DESC: BLI_LI - Turn signal left, OFFSET: 39, LEN: 1
  bool BLI_RE                     : 1; // DESC: BLI_RE - Turn signal right, OFFSET: 38, LEN: 1
  bool                            : 1; // Padding, OFFSET: 37, LEN: 1
  bool KL_31B                     : 1; // DESC: KL_31B - Wiper out of park position, OFFSET: 36, LEN: 1
  bool HAS_KL                     : 1; // DESC: HAS_KL - Handbrake applied (indicator lamp), OFFSET: 35, LEN: 1
  uint8_t ESP_BET                 : 2; // DESC: ESP_BET - ESP on/off actuated, OFFSET: 33, LEN: 2
/*
  enum class ESP_BET_c : uint8_t {     // DESC: ESP_BET - ESP on/off actuated, OFFSET: 33, LEN: 2
    NBET                          = 0, // enum: 0 - Not operated (rocker and push push) / NBET
    AUS_BET                       = 1, // enum: 1 - ESP off actuated (rocker), actuated (push push) / AUS_BET
    EIN_NDEF                      = 2, // enum: 2 - ESP on actuated (rocker), not defined (push push) / EIN_NDEF
    SNV                           = 3, // enum: 3 - No signal (rocker and push push) / SNV
  } : 2; ESP_BET_c ESP_BET;
*/
  bool BN_NTLF                    : 1; // DESC: BN_NTLF - Vehicle power supply emergency mode: Prio1 and Prio2 consumers off, second battery supports, OFFSET: 32, LEN: 1
// byte 5
  bool KL54_RM                    : 1; // DESC: KL54_RM - Terminal 54 hardware active, OFFSET: 47, LEN: 1
  bool ABL_EIN                    : 1; // DESC: ABL_EIN - Turn on low beam, OFFSET: 46, LEN: 1
  uint8_t ART_ABW_BET             : 2; // DESC: ART_ABW_BET - ART distance warning on/off actuated, OFFSET: 44, LEN: 2
/*
  enum class ART_ABW_BET_c : uint8_t { // DESC: ART_ABW_BET - ART distance warning on/off actuated, OFFSET: 44, LEN: 2
    NDEF_NBET                     = 0, // enum: 0 - not defined (rocker), not actuated (push push) / NDEF_NBET
    AUS_NDEF                      = 1, // enum: 1 - distance warning off (rocker), not defined (push push) / AUS_NDEF
    EIN_BET                       = 2, // enum: 2 - Distance warning on (rocker), actuated (push push) / EIN_BET
    SNV                           = 3, // enum: 3 - No signal (rocker and push push) / SNV
  } : 2; ART_ABW_BET_c ART_ABW_BET;
*/
  uint8_t ST3_BET                 : 2; // DESC: ST3_BET - LF/ABC 3-position switch operated, OFFSET: 42, LEN: 2
/*
  enum class ST3_BET_c : uint8_t {     // DESC: ST3_BET - LF/ABC 3-position switch operated, OFFSET: 42, LEN: 2
    NBET                          = 0, // enum: 0 - Not operated (rocker and push push) / NBET
    UNBET_NDEF                    = 1, // enum: 1 - Bottom Actuated (Rocker), Undefined (Push Push) / UNBET_NDEF
    OBBET_BET                     = 2, // enum: 2 - Top Actuated (Rocker), Actuated (Push Push) / OBBET_BET
    NDEF                          = 3, // enum: 3 - Undefined / NDEF
  } : 2; ST3_BET_c ST3_BET;
*/
  uint8_t ST2_BET                 : 2; // DESC: ST2_BET - LF/ABC 2-position switch actuated, OFFSET: 40, LEN: 2
/*
  enum class ST2_BET_c : uint8_t {     // DESC: ST2_BET - LF/ABC 2-position switch actuated, OFFSET: 40, LEN: 2
    NBET                          = 0, // enum: 0 - Not operated (rocker and push push) / NBET
    UNBET_NDEF                    = 1, // enum: 1 - Bottom Actuated (Rocker), Undefined (Push Push) / UNBET_NDEF
    OBBET_BET                     = 2, // enum: 2 - Top Actuated (Rocker), Actuated (Push Push) / OBBET_BET
    NDEF                          = 3, // enum: 3 - Undefined / NDEF
  } : 2; ST2_BET_c ST2_BET;
*/
// byte 6
  uint8_t ART_ABSTAND             : 8; // DESC: ART_ABSTAND - distance factor (multiplier: 1.0, addend: 0.0), OFFSET: 48, LEN: 8
// byte 7
  uint8_t LDC                     : 2; // DESC: LDC - country code, OFFSET: 62, LEN: 2
/*
  enum class LDC_c : uint8_t {         // DESC: LDC - country code, OFFSET: 62, LEN: 2
    RDW                           = 0, // enum: 0 - Rest of the world / RDW
    USA_CAN                       = 1, // enum: 1 - USA/Canada / USA_CAN
    NDEF                          = 2, // enum: 2 - Undefined / NDEF
    SNV                           = 3, // enum: 3 - Signal not available / SNV
  } : 2; LDC_c LDC;
*/
  uint8_t FZGVERSN                : 3; // DESC: FZGVERSN - Series-dependent vehicle version (only 220/215/230), OFFSET: 59, LEN: 3
/*
  enum class FZGVERSN_c : uint8_t {    // DESC: FZGVERSN - Series-dependent vehicle version (only 220/215/230), OFFSET: 59, LEN: 3
    START                         = 0, // enum: 0 - Status at market launch of the respective series / START
    V1                            = 1, // enum: 1 - BR 220: AJ 99/X, C215: AJ 01/1, R230: AJ 02/1 / V1
    V2                            = 2, // enum: 2 - BR 220: AJ 01/1, C215: AJ 02/X, R230: AJ 03/X / V2
    V3                            = 3, // enum: 3 - BR 220: ÄJ 02/X, C215: ÄJ 03/X, R230: not defined / V3
    V4                            = 4, // enum: 4 - BR 220: prohibited, C215/R230: not defined / V4
    V5                            = 5, // enum: 5 - BR 220: prohibited, C215/R230: not defined / V5
    V6                            = 6, // enum: 6 - BR 220: ÄJ 03/X, C215,/R230: not defined / V6
    V7                            = 7, // enum: 7 - BR 220/ C215,/R230: not defined / V7
  } : 3; FZGVERSN_c FZGVERSN;
*/
  bool                            : 1; // Padding, OFFSET: 58, LEN: 1
  bool GBL_AUS                    : 1; // DESC: GBL_AUS - E-suction fan: Basic ventilation off, OFFSET: 57, LEN: 1
  bool ART_VH                     : 1; // DESC: ART_VH - ART available, OFFSET: 56, LEN: 1
} __attribute__((__packed__)) EZS_240h_t;


// ECU: LF_ABC, NAME: FS_340h, ID: 0x0340, MSG COUNT: 16
typedef struct FS_340h_t {
// byte 0
  bool FM1                        : 1; // DESC: FM1 - Error 1: "Stop car too low", OFFSET: 7, LEN: 1
  bool FM2                        : 1; // DESC: FM2 - Error 2: "wait a moment" (LF)/ "steering oil" (only ABC), OFFSET: 6, LEN: 1
  bool FM3                        : 1; // DESC: FM3 - Error 3: "Visit workshop", OFFSET: 5, LEN: 1
  bool FM4                        : 1; // DESC: FM4 - Error 4: "Park vehicle", OFFSET: 4, LEN: 1
  bool M1                         : 1; // DESC: M1 - Message 1: "Vehicle lifts", BR164/251: "Highway->Offroad", BR164 Offroad: "Highway->Offroad1", OFFSET: 3, LEN: 1
  bool M2                         : 1; // DESC: M2 - Message 2: "Level selection deleted", OFFSET: 2, LEN: 1
  bool NEDG                       : 1; // DESC: NEDG - Level calibration performed, OFFSET: 1, LEN: 1
  bool                            : 1; // Padding, OFFSET: 0, LEN: 1
// byte 1
  bool ST3_LEDL_DL                : 1; // DESC: ST3_LEDL_DL - Left LED 3-position switch steady light (164/251 lower LED), OFFSET: 15, LEN: 1
  bool                            : 1; // Padding, OFFSET: 14, LEN: 1
  bool ST3_LEDR_DL                : 1; // DESC: ST3_LEDR_DL - Right LED 3-position switch steady light (164/251 top LED), OFFSET: 13, LEN: 1
  bool                            : 1; // Padding, OFFSET: 12, LEN: 1
  bool ST2_LED_DL                 : 1; // DESC: ST2_LED_DL - LED 2-stage switch steady light, OFFSET: 11, LEN: 1
  uint8_t BELAD                   : 2; // DESC: BELAD - loading, OFFSET: 9, LEN: 2
/*
  enum class BELAD_c : uint8_t {       // DESC: BELAD - loading, OFFSET: 9, LEN: 2
    LEER                          = 0, // enum: 0 - Unloaded / LEER
    HALB                          = 1, // enum: 1 - Half loaded / HALB
    VOLL                          = 2, // enum: 2 - Fully loaded / VOLL
    SNV                           = 3, // enum: 3 - Load not recognized / SNV
  } : 2; BELAD_c BELAD;
*/
  bool                            : 1; // Padding, OFFSET: 8, LEN: 1
// byte 2-6
  uint8_t FZGN_VL                 : 8; // DESC: FZGN_VL - Vehicle level, front left (multiplier: 1.0, addend: 0.0), OFFSET: 16, LEN: 8
  uint8_t FZGN_VR                 : 8; // DESC: FZGN_VR - Vehicle level, front right (multiplier: 1.0, addend: 0.0), OFFSET: 24, LEN: 8
  uint8_t FZGN_HL                 : 8; // DESC: FZGN_HL - Rear left vehicle level (multiplier: 1.0, addend: 0.0), OFFSET: 32, LEN: 8
  uint8_t FZGN_HR                 : 8; // DESC: FZGN_HR - Vehicle level, rear right (multiplier: 1.0, addend: 0.0), OFFSET: 40, LEN: 8
  uint8_t                         : 8; // Padding, OFFSET: 48, LEN: 8
// byte 7
  uint8_t                         : 5; // Padding, OFFSET: 59, LEN: 5
  uint8_t FS_ID                   : 3; // DESC: FS_ID - Suspension control identification, OFFSET: 56, LEN: 3
/*
  enum class FS_ID_c : uint8_t {       // DESC: FS_ID - Suspension control identification, OFFSET: 56, LEN: 3
    LF                            = 0, // enum: 0 - Air suspension/ LF (BR164/251 NR without ADS) / LF
    SLF                           = 1, // enum: 1 - Semi-active air suspension, SLF (BR164/251 NR+ADS) / SLF
    EHNR                          = 2, // enum: 2 - Electronic rear axle level control / EHNR
    ABC                           = 3, // enum: 3 - Active Body Control 1 / ABC
  } : 3; FS_ID_c FS_ID;
*/
} __attribute__((__packed__)) FS_340h_t;

#endif /* W211_CAN_C_H */