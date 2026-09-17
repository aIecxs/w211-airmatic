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


// ECU: ZGW, NAME: ZGW_248h, ID: 0x0248, MSG COUNT: 31
typedef struct ZGW_248h_t {
  // byte 0
  uint8_t ST3_BET                 : 2; // DESC: ST3_BET - LF/ABC 3-position switch operated, OFFSET: 6, LEN: 2
/*
  enum class ST3_BET_c : uint8_t {     // DESC: ST3_BET - LF/ABC 3-position switch operated, OFFSET: 6, LEN: 2
    NBET                          = 0, // enum: 0 - Not operated (rocker and push push) / NBET
    UNBET_NDEF                    = 1, // enum: 1 - Bottom Actuated (Rocker), Undefined (Push Push) / UNBET_NDEF
    OBBET_BET                     = 2, // enum: 2 - Top Actuated (Rocker), Actuated (Push Push) / OBBET_BET
    NDEF                          = 3, // enum: 3 - Undefined / NDEF
  } : 2; ST3_BET_c ST3_BET;
*/
  uint8_t ST2_BET                 : 2; // DESC: ST2_BET - LF/ABC 2-position switch actuated, OFFSET: 4, LEN: 2
/*
  enum class ST2_BET_c : uint8_t {     // DESC: ST2_BET - LF/ABC 2-position switch actuated, OFFSET: 4, LEN: 2
    NBET                          = 0, // enum: 0 - Not operated (rocker and push push) / NBET
    UNBET_NDEF                    = 1, // enum: 1 - Bottom Actuated (Rocker), Undefined (Push Push) / UNBET_NDEF
    OBBET_BET                     = 2, // enum: 2 - Top Actuated (Rocker), Actuated (Push Push) / OBBET_BET
    NDEF                          = 3, // enum: 3 - Undefined / NDEF
  } : 2; ST2_BET_c ST2_BET;
*/
  bool ABL_EIN                    : 1; // DESC: ABL_EIN - Turn on low beam, OFFSET: 3, LEN: 1
  bool                            : 1; // Padding, OFFSET: 2, LEN: 1
  bool DIAG_X4_F                  : 1; // DESC: Start Xenon4 diagnostic procedure driver's side, OFFSET: 1, LEN: 1
  bool DIAG_X4_B                  : 1; // DESC: Start Xenon4 diagnostic procedure passenger side, OFFSET: 0, LEN: 1
  // byte 1
  uint8_t ANH_ERK2                : 2; // DESC: ANH_ERK2 - Trailer operation detected, OFFSET: 14, LEN: 2
/*
  enum class ANH_ERK2_c : uint8_t {    // DESC: ANH_ERK2 - Trailer operation detected, OFFSET: 14, LEN: 2
    KEIN                          = 0, // enum: 0 - Trailer not recognized / KEIN
    OK                            = 1, // enum: 1 - Trailer detected / OK
    NDEF                          = 2, // enum: 2 - Undefined / NDEF
    SNV                           = 3, // enum: 3 - Signal not available / SNV
  } : 2; ANH_ERK2_c ANH_ERK2;
*/
  bool ZWP_LFT                    : 1; // DESC: Auxiliary water pump is running, OFFSET: 13, LEN: 1
  bool AFL_ABL_EIN                : 1; // DESC: AFL request: Turn on low beam, OFFSET: 12, LEN: 1
  bool MOT_AUF                    : 1; // DESC: Bonnet is up, OFFSET: 11, LEN: 1
  bool                            : 1; // Padding, OFFSET: 10, LEN: 1
  bool GWHST_P_SAM                : 1; // DESC: Transmission selector lever position "P" detected, OFFSET: 9, LEN: 1
  bool BN_SOCS                    : 1; // DESC: Vehicle electrical system warning: starter battery state of charge, OFFSET: 8, LEN: 1
  // byte 2
  bool BLS_A                      : 1; // DESC: SAM/x: brake light switch output EHB-ASG, x = B (230), V (211), F (240), OFFSET: 23, LEN: 1
  bool VSTAT_A                    : 1; // DESC: SAM/x: v-signal from EHB-ASG, x = B (230), V (211), F ( 240), OFFSET: 22, LEN: 1
  bool INF_RFE_SAM                : 1; // DESC: SAM/x: EHB-ASG in fallback level, x = B (230), V (211,164,251), F (240), OFFSET: 21, LEN: 1
  bool SAM_PAS                    : 1; // DESC: SAM/x passive, x = Bb (230), V (211), F (240), OFFSET: 20, LEN: 1
  uint8_t                         : 4; // Padding, OFFSET: 16, LEN: 4
  // byte 3
  bool MW_AKT_SAM                 : 1; // DESC: SBC added value: run-on active, OFFSET: 31, LEN: 1
  bool BNS_AKT                    : 1; // DESC: Onboard power supply control unit active, OFFSET: 30, LEN: 1
  bool KL_31B                     : 1; // DESC: Wiper out of park position, OFFSET: 29, LEN: 1
  bool HAS_KL                     : 1; // DESC: Handbrake applied (indicator lamp), OFFSET: 28, LEN: 1
  uint8_t ESP_BET                 : 2; // DESC: ESP_BET - ESP on/off actuated, OFFSET: 26, LEN: 2
/*
  enum class ESP_BET_c : uint8_t {     // DESC: ESP_BET - ESP on/off actuated, OFFSET: 26, LEN: 2
    NBET                          = 0, // enum: 0 - Not operated (rocker and push push) / NBET
    AUS_BET                       = 1, // enum: 1 - ESP off actuated (rocker), actuated (push push) / AUS_BET
    EIN_NDEF                      = 2, // enum: 2 - ESP on actuated (rocker), not defined (push push) / EIN_NDEF
    SNV                           = 3, // enum: 3 - No signal (rocker and push push) / SNV
  } : 2; ESP_BET_c ESP_BET;
*/
  bool BN_NTLF                    : 1; // DESC: Vehicle power supply emergency mode: Prio1 and Prio2 consumers off, second battery supports, OFFSET: 25, LEN: 1
  bool KL54_RM                    : 1; // DESC: Terminal 54 hardware active, OFFSET: 24, LEN: 1
  // byte 4
  uint8_t OC_BF                   : 3; // DESC: OC_BF - Occupant classification passenger, OFFSET: 37, LEN: 3
/*
  enum class OC_BF_c : uint8_t {       // DESC: OC_BF - Occupant classification passenger, OFFSET: 37, LEN: 3
    OC_0                          = 0, // enum: 0 - Occupant classification 0 / OC_0
    OC_1                          = 1, // enum: 1 - Occupant Classification 1 / OC_1
    OC_2                          = 2, // enum: 2 - Occupant classification 2 / OC_2
    OC_3                          = 3, // enum: 3 - Occupant classification 3 / OC_3
    OC_4                          = 4, // enum: 4 - Occupant classification 4 / OC_4
    OC_FEHLER                     = 6, // enum: 6 - Error OC / OC_FEHLER
    SNV                           = 7, // enum: 7 - Signal not available / SNV
  } : 3; OC_BF_c OC_BF;
*/
  bool                            : 1; // Padding, OFFSET: 36, LEN: 1
  uint8_t GS_F                    : 2; // DESC: GS_F - Seat belt buckle driver, OFFSET: 34, LEN: 2
/*
  enum class GS_F_c : uint8_t {        // DESC: GS_F - Seat belt buckle driver, OFFSET: 34, LEN: 2
    GS_OK                         = 0, // enum: 0 - Seat belt buckle inserted / GS_OK
    GS_NOK                        = 1, // enum: 1 - Seat belt buckle not inserted / GS_NOK
    GS_FEHLER                     = 2, // enum: 2 - Seat belt buckle error / GS_FEHLER
    SNV                           = 3, // enum: 3 - Signal not available / SNV
  } : 2; GS_F_c GS_F;
*/
  uint8_t GS_BF                   : 2; // DESC: GS_BF - Passenger seat belt buckle, OFFSET: 32, LEN: 2
/*
  enum class GS_BF_c : uint8_t {       // DESC: GS_BF - Passenger seat belt buckle, OFFSET: 32, LEN: 2
    GS_OK                         = 0, // enum: 0 - Seat belt buckle inserted / GS_OK
    GS_NOK                        = 1, // enum: 1 - Seat belt buckle not inserted / GS_NOK
    GS_FEHLER                     = 2, // enum: 2 - Seat belt buckle error / GS_FEHLER
    SNV                           = 3, // enum: 3 - Signal not available / SNV
  } : 2; GS_BF_c GS_BF;
*/
  // byte 5
  bool X_CRASH                    : 1; // DESC: Any crash event, OFFSET: 47, LEN: 1
  bool CRASH_CNF                  : 1; // DESC: CRASH confirm bit, OFFSET: 46, LEN: 1
  bool CRASH                      : 1; // DESC: Crash signal from airbag SG, OFFSET: 45, LEN: 1
  bool                            : 1; // Padding, OFFSET: 44, LEN: 1
  uint8_t PTS_BET                 : 2; // DESC: PTS_BET - Parctronic operated completely on/off, OFFSET: 42, LEN: 2
/*
  enum class PTS_BET_c : uint8_t {     // DESC: PTS_BET - Parctronic operated completely on/off, OFFSET: 42, LEN: 2
    NBET                          = 0, // enum: 0 - Not activated / NBET
    NDEF                          = 1, // enum: 1 - Undefined / NDEF
    BET                           = 2, // enum: 2 - Confirmed / BET
    SNV                           = 3, // enum: 3 - Signal not available / SNV
  } : 2; PTS_BET_c PTS_BET;
*/
  bool                            : 1; // Padding, OFFSET: 41, LEN: 1
  bool KOMBI_MW_OK                : 1; // DESC: Kombi is value-added, OFFSET: 40, LEN: 1
  // byte 6
  uint8_t                         : 8; // Padding, OFFSET: 48, LEN: 8
  // byte 7
  uint8_t                         : 7; // Padding, OFFSET: 57, LEN: 7
  bool ART_VH                     : 1; // DESC: ART available, OFFSET: 56, LEN: 1
} __attribute__((__packed__)) ZGW_248h_t;


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


// ECU: MS, NAME: MS_608h, ID: 0x0608, MSG COUNT: 13
typedef struct MS_608h_t {
// byte 0-1
  uint8_t T_MOT                   : 8; // DESC: T_MOT - engine coolant temperature, OFFSET: 0, LEN: 8
  uint8_t T_LUFT                  : 8; // DESC: T_LUFT - intake air temperature, OFFSET: 8, LEN: 8
// byte 2
  uint8_t FCOD_KAR                : 3; // DESC: FCOD_KAR - vehicle code body, OFFSET: 16, LEN: 3
/*
  enum class FCOD_KAR_c : uint8_t {    // DESC: FCOD_KAR - vehicle code body, OFFSET: 16, LEN: 3
    W                             = 0, // enum: 0 - Limousine (or G for short BM1 / 3 for BR 463, G for 461) / W
    V                             = 1, // enum: 1 - sedan long (or VF with BR 210/211, G long BM6 with BR 463) / V
    C                             = 2, // enum: 2 - Coupé (or VV for BR 210/211/220) / C
    S                             = 3, // enum: 3 - T-Model (or special protection B4 for BR W240, T for BR 245) / S
    A                             = 4, // enum: 4 - Cabrio (or X with BR 164) / A
    R                             = 5, // enum: 5 - Roadster (or special protection B4 for BR 210/211/220 / V240) / R
    SS                            = 6, // enum: 6 - special protection B6 / 7 (or CL for BR 203) / SS
    SNV                           = 7, // enum: 7 - code does not exist / SNV
  } : 3; FCOD_KAR_c FCOD_KAR;
*/
  uint8_t FCOD_BR                 : 5; // DESC: FCOD_BR - vehicle code series, OFFSET: 19, LEN: 5
/*
  enum class FCOD_BR_c : uint8_t {     // DESC: FCOD_BR - vehicle code series, OFFSET: 19, LEN: 5
    BR221                         = 0, // enum: 0 - BR 221 or BR 140 / BR221
    BR129                         = 1, // enum: 1 - BR 129 / BR129
    BR210                         = 2, // enum: 2 - BR 210 or BR 212 / BR210
    BR202                         = 3, // enum: 3 - BR 202 or BR 204 / BR202
    BR220                         = 4, // enum: 4 - BR 220 / BR220
    BR170                         = 5, // enum: 5 - BR 170 / BR170
    BR203                         = 6, // enum: 6 - BR 203 / BR203
    BR168                         = 7, // enum: 7 - BR 168 / BR168
    BR163                         = 8, // enum: 8 - BR 163 / BR163
    BR208                         = 9, // enum: 9 - BR 208 / BR208
    BR463                         = 10, // enum: 10 - BR 463 / BR463
    BR215                         = 11, // enum: 11 - BR 215 / BR215
    BR230                         = 12, // enum: 12 - BR 230 / BR230
    BR211                         = 13, // enum: 13 - BR 211 / BR211
    BR209                         = 14, // enum: 14 - BR 209 / BR209
    BR461                         = 15, // enum: 15 - BR 461 / BR461
    BR240                         = 16, // enum: 16 - BR 240 / BR240
    BR251                         = 17, // enum: 17 - BR 251 / BR251
    BR171                         = 18, // enum: 18 - BR 171 / BR171
    BR164                         = 19, // enum: 19 - BR 164 / BR164
    BR169                         = 20, // enum: 20 - BR 169 or BR 245 / BR169
    BR199                         = 21, // enum: 21 - BR 199 / BR199
    BR216                         = 22, // enum: 22 - BR 216 / BR216
    BR219                         = 23, // enum: 23 - BR 219 / BR219
    BR454                         = 24, // enum: 24 - BR 454 (z-car) / BR454
    NCV2                          = 25, // enum: 25 - NCV2 / NCV2
    VITO                          = 26, // enum: 26 - V-Class / Vito / VITO
    SPRINTER                      = 27, // enum: 27 - Sprinter / SPRINTER
    NCV3                          = 28, // enum: 28 - NCV3 / NCV3
    NCV1                          = 29, // enum: 29 - NCV1 / NCV1
    REST                          = 30, // enum: 30 - all other BR / REST
    SNV                           = 31, // enum: 31 - code does not exist / SNV
  } : 5; FCOD_BR_c FCOD_BR;
*/
// byte 3
  bool FCOD_MOT6                  : 1; // DESC: FCOD_MOT6 - Vehicle code engine with 7 bits, bit 6, OFFSET: 24, LEN: 1
  bool GS_NVH                     : 1; // DESC: GS_NVH - transmission control not available, OFFSET: 25, LEN: 1
  uint8_t FCOD_MOT0               : 6; // DESC: FCOD_MOT - Vehicle code engine 7Bit, Bit0-5 (Bit6 -> Signal FCOD_MOT6), OFFSET: 26, LEN: 6
/*
  enum class FCOD_MOT0_c : uint8_t {
	// ... section extra bytes -> FCOD_MOT
  } : 6; FCOD_MOT_c FCOD_MOT0;
*/
// byte 4
  uint8_t V_MAX_FIX               : 8; // DESC: V_MAX_FIX - Fixed maximum speed, OFFSET: 32, LEN: 8
// byte 5+6
  uint16_t VB                     : 16; // DESC: VB - consumption, OFFSET: 40, LEN: 16
// byte 7
  bool ZWP_EIN_MS                 : 1; // DESC: ZWP_EIN_MS - switch on additional water pump, OFFSET: 56, LEN: 1
  uint8_t PFW                     : 2; // DESC: PFW - Particle filter warning, OFFSET: 57, LEN: 2
/*
  enum class PFW_c : uint8_t {         // DESC: PFW - Particle filter warning, OFFSET: 57, LEN: 2
    OK                            = 0, // enum: 0 - No warning / OK
    PFW1                          = 1, // enum: 1 - Warning Particle filter closed, level 1 / PFW1
    PFW2                          = 2, // enum: 2 - Warning Particle filter closed, level 2 / PFW2
    SNV                           = 3, // enum: 3 - signal not available / SNV
  } : 2; PFW_c PFW;
*/
  bool ZVB_EIN_MS                 : 1; // DESC: ZVB_EIN_MS - switch on additional consumer, OFFSET: 59, LEN: 1
  uint8_t PFKO                    : 4; // DESC: PFKO - Particle filter correction offset FMMOTMAX, OFFSET: 60, LEN: 4
/* <- End of CAN Frame */

// extra bytes -> must set extern
// byte 8
//  uint8_t                         : 1; // Padding, OFFSET: 64, LEN: 1
//  uint8_t FCOD_MOT                : 7; // DESC: FCOD_MOT - Vehicle code engine 7Bit, (concatenated FCOD_MOT0 + FCOD_MOT6)
/*
  enum class FCOD_MOT_c : uint8_t {    // DESC: FCOD_MOT - Vehicle code engine 7Bit, (concatenated FCOD_MOT0 + FCOD_MOT6)
    M272E35                       = 0, // enum: 0 - M272 E35 / M272E35
    M271E18ML105                  = 1, // enum: 1 - M271 E18 ML red. (105 kW) / M271E18ML105
    M271E18ML120                  = 2, // enum: 2 - M271 E18 ML (120 kW) / M271E18ML120
    M112E37                       = 3, // enum: 3 - M112 E37 / M112E37
    M272E25                       = 4, // enum: 4 - M272 E25 / M272E25
    M272E30                       = 5, // enum: 5 - M272 E30 / M272E30
    M112E28                       = 7, // enum: 7 - M112 E28 / M112E28
    M112E32                       = 8, // enum: 8 - M112 E32 / M112E32
    M273E46                       = 10, // enum: 10 - M273 E46 / M273E46
    M273E55                       = 11, // enum: 11 - M273 E55 / M273E55
    M112E26                       = 12, // enum: 12 - M112 E26 / M112E26
    M113E43                       = 13, // enum: 13 - M113 E43 / M113E43
    M113E50                       = 14, // enum: 14 - M113 E50 / M113E50
    M271E18ML140                  = 18, // enum: 18 - M271 E18 ML / 1 (140 kW) / M271E18ML140
    M271DE18ML105                 = 19, // enum: 19 - M271 DE18 ML red. (105 kW) / M271DE18ML105
    M271DE18ML125                 = 20, // enum: 20 - M271 DE18 ML (125 kW) / M271DE18ML125
    M111E_E23ML                   = 22, // enum: 22 - M111E E23 ML / M111E_E23ML
    M111E_E20                     = 23, // enum: 23 - M111E E20 / M111E_E20
    M111E_E20ML                   = 24, // enum: 24 - M111E E20 ML / M111E_E20ML
    M112E32_140                   = 25, // enum: 25 - M112 E32 red. (140 kW) / M112E32_140
    M266E20ATL                    = 26, // enum: 26 - M266 E20 ATL / M266E20ATL
    M266E15                       = 27, // enum: 27 - M266 E15 / M266E15
    M266E17                       = 28, // enum: 28 - M266 E17 / M266E17
    M266E20                       = 29, // enum: 29 - M266 E20 / M266E20
    M275E55                       = 30, // enum: 30 - M275 E55 or M285 E55 / M275E55
    M137E58                       = 31, // enum: 31 - M137 E58 / M137E58
    OM640DE20LA60                 = 32, // enum: 32 - OM 640 DE20 LA (60 kW) / OM640DE20LA60
    OM640DE20LA80                 = 34, // enum: 34 - OM 640 DE20 LA (80 kW) / OM640DE20LA80
    OM642DE30LA160                = 35, // enum: 35 - OM642 DE30 LA (155/160 kW) / OM642DE30LA160
    OM640DE20LA100                = 36, // enum: 36 - OM 640 DE20 LA (100 kW) / OM640DE20LA100
    OM613DE32LA                   = 37, // enum: 37 - OM613 DE32 LA or OM648 DE32 LA / OM613DE32LA
    OM628DE40LA                   = 39, // enum: 39 - OM628 DE40 LA / OM628DE40LA
    OM642DE30LA140                = 40, // enum: 40 - OM642 DE30 LA (140 kW) / OM642DE30LA140
    OM612DE27LA                   = 43, // enum: 43 - OM612 DE27 LA or OM647 DE27 LA (120/130 kW) / OM612DE27LA
    OM611DE22LA100                = 44, // enum: 44 - OM611 DE22 LA (105/100 kW) or OM646 DE22 LA (100/105/110 kW) / OM611DE22LA100
    OM611DE22LA85                 = 45, // enum: 45 - OM611 DE22 LA (85 kW) or OM646 DE22 LA (90 kW) / OM611DE22LA85
    OM611DE22LA75                 = 46, // enum: 46 - OM611 DE22 LA (75 kW) or OM646 DE22 LA (75 kW) / OM611DE22LA75
    M134E11                       = 64, // enum: 64 - M134 E11 (3A91) / M134E11
    M135E13                       = 65, // enum: 65 - M135 E13 (4A90) / M135E13
    M135E15                       = 66, // enum: 66 - M135 E15 (4A91) / M135E15
    M135E15ATL                    = 67, // enum: 67 - M135 E15 ATL / M135E15ATL
    M272DE25                      = 68, // enum: 68 - M272 DE25 / M272DE25
    M272DE30                      = 69, // enum: 69 - M272 DE30 / M272DE30
    M272DE35                      = 70, // enum: 70 - M272 DE35 / M272DE35
    M273DE46                      = 71, // enum: 71 - M273 DE46 / M273DE46
    M273DE55                      = 72, // enum: 72 - M273 DE55 / M273DE55
    M271E18MLATTR115              = 79, // enum: 79 - M271 E18 ML Attr. (115kW) / M271E18MLATTR115
    M271E18MLATTR141              = 80, // enum: 80 - M271 E18 ML Attr. (141kW) / M271E18MLATTR141
    OM629DE40LA                   = 96, // enum: 96 - OM629 DE40 LA / OM629DE40LA
    OM642DE30LARED140             = 99, // enum: 99 - OM642 DE30 LA red. (140kW) / OM642DE30LARED140
  } : 7; FCOD_MOT_c FCOD_MOT;
*/
} __attribute__((__packed__)) MS_608h_t;


#endif /* W211_CAN_C_H */