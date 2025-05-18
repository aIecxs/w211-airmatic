#ifndef W211_CAN_B_H
#define W211_CAN_B_H


// ECU NAME: KOMBI_A5, ID: 0x01CA, MSG COUNT: 25
typedef struct KOMBI_A5_t {
  uint8_t KI_STAT                 : 8; // MSG NAME: KI_STAT - Status Kombi, OFFSET 0, LENGTH 8
/*
  enum class KI_STAT_c : uint8_t {     // MSG NAME: KI_STAT - Status Kombi, OFFSET 0, LENGTH 8
//    reserviert                    = 0, // enum: 0 - reserviert
//    reserviert                    = 1, // enum: 1 - reserviert
    Neutral                       = 2, // enum: 2 - Neutral
    AUDIO                         = 3, // enum: 3 - Audio / AUDIO
    NAVI                          = 4, // enum: 4 - Navigation / NAVI
    TEL                           = 5, // enum: 5 - Telefon / TEL
    NEU_SER                       = 6, // enum: 6 - Neue Services / NEU_SER
    SPR_FNK_DLG_CLO              = 19, // enum: 19 - Sprachfunk-KI Dlg geschl / SPR_FNK_DLG_CLO
    DAT_FNK_DLG_CLO              = 20, // enum: 20 - Datenfunk-KI Dlg geschl / DAT_FNK_DLG_CLO
    SPR_FNK_DLG_OPN              = 21, // enum: 21 - Sprachfunk-KI Dlg geöffnet / SPR_FNK_DLG_OPN
    DAT_FNK_DLG_OPN              = 22, // enum: 22 - Datenfunk-KI Dlg geöffnet / DAT_FNK_DLG_OPN
    SNV                         = 255, // enum: 255 - Signal nicht vorhanden / SNV
  } : 8; KI_STAT_c KI_STAT;
*/
  bool BUTTON_1_1                 : 1; // MSG NAME: BUTTON_1_1 - Nächstes Display, OFFSET 15, LENGTH 1
  bool BUTTON_1_2                 : 1; // MSG NAME: BUTTON_1_2 - Vorheriges Display, OFFSET 14, LENGTH 1
  bool BUTTON_2_1                 : 1; // MSG NAME: BUTTON_2_1 - Reserve, OFFSET 13, LENGTH 1
  bool BUTTON_2_2                 : 1; // MSG NAME: BUTTON_2_2 - Reserve, OFFSET 12, LENGTH 1
  bool BUTTON_3_1                 : 1; // MSG NAME: BUTTON_3_1 - Taste "+", OFFSET 11, LENGTH 1
  bool BUTTON_3_2                 : 1; // MSG NAME: BUTTON_3_2 - Taste "-", OFFSET 10, LENGTH 1
  bool BUTTON_4_1                 : 1; // MSG NAME: BUTTON_4_1 - Telefon Send, OFFSET 9, LENGTH 1
  bool BUTTON_4_2                 : 1; // MSG NAME: BUTTON_4_2 - Telefon End, OFFSET 8, LENGTH 1
  bool BUTTON_5_1                 : 1; // MSG NAME: BUTTON_5_1 - Reserve, OFFSET 23, LENGTH 1
  bool BUTTON_5_2                 : 1; // MSG NAME: BUTTON_5_2 - Reserve, OFFSET 22, LENGTH 1
  bool BUTTON_6_1                 : 1; // MSG NAME: BUTTON_6_1 - Reserve, OFFSET 21, LENGTH 1
  bool BUTTON_6_2                 : 1; // MSG NAME: BUTTON_6_2 - Reserve, OFFSET 20, LENGTH 1
  bool BUTTON_7_1                 : 1; // MSG NAME: BUTTON_7_1 - Reserve, OFFSET 19, LENGTH 1
  bool BUTTON_7_2                 : 1; // MSG NAME: BUTTON_7_2 - Reserve, OFFSET 18, LENGTH 1
  bool BUTTON_8_1                 : 1; // MSG NAME: BUTTON_8_1 - Reserve, OFFSET 17, LENGTH 1
  bool BUTTON_8_2                 : 1; // MSG NAME: BUTTON_8_2 - Reserve, OFFSET 16, LENGTH 1
  bool PTT_1_1                    : 1; // MSG NAME: PTT_1_1 - Linguatronic aktivieren, OFFSET 31, LENGTH 1
  bool PTT_1_2                    : 1; // MSG NAME: PTT_1_2 - Linguatronic deaktivieren, OFFSET 30, LENGTH 1
  bool PTT_2_1                    : 1; // MSG NAME: PTT_2_1 - Reserve, OFFSET 29, LENGTH 1
  bool PTT_2_2                    : 1; // MSG NAME: PTT_2_2 - Reserve, OFFSET 28, LENGTH 1
  bool PTT_3_1                    : 1; // MSG NAME: PTT_3_1 - Reserve, OFFSET 27, LENGTH 1
  bool PTT_3_2                    : 1; // MSG NAME: PTT_3_2 - Reserve, OFFSET 26, LENGTH 1
  bool PTT_4_1                    : 1; // MSG NAME: PTT_4_1 - Reserve, OFFSET 25, LENGTH 1
  bool PTT_4_2                    : 1; // MSG NAME: PTT_4_2 - Reserve, OFFSET 24, LENGTH 1
} __attribute__((__packed__)) KOMBI_A5_t;


// ECU NAME: UBF_A1, ID: 0x001A, MSG COUNT: 9
typedef struct UBF_A1_t {
  bool FU_FRSP_BET                : 1; // MSG NAME: FU_FRSP_BET - Taster Funkaufschaltung betätigt, OFFSET 7, LENGTH 1
  uint8_t                         : 3; // Padding, OFFSET: 4, LEN: 3
  uint8_t ART_ABW_BET             : 2; // MSG NAME: ART_ABW_BET - ART-Abstandswarnung ein/aus betätigt, OFFSET 2, LENGTH 2
/*
  enum class ART_ABW_BET_c : uint8_t { // MSG NAME: ART_ABW_BET - ART-Abstandswarnung ein/aus betätigt, OFFSET 2, LENGTH 2
    NDEF_NBET                     = 0, // enum: 0 - nicht definiert (Wippe), Nicht betätigt (Push Push) / NDEF_NBET
    AUS_NDEF                      = 1, // enum: 1 - Abstandswarnung aus (Wippe), nicht definiert (Push Push) / AUS_NDEF
    EIN_BET                       = 2, // enum: 2 - Abstandswarnung ein (Wippe), Betätigt (Push Push) / EIN_BET
    SNV                           = 3, // enum: 3 - Signal nicht vorhanden (Wippe und Push Push) / SNV
  } : 2; ART_ABW_BET_c ART_ABW_BET;
*/
  uint8_t                         : 2; // Padding, OFFSET: 0, LEN: 2
  uint8_t ART_ABSTAND             : 8; // MSG NAME: ART_ABSTAND - Abstandsfaktor, OFFSET 8, LENGTH 8
  bool                            : 1; // Padding, OFFSET: 23, LEN: 1
  bool ST3_BET                    : 1; // MSG NAME: ST3_BET - 3-stufiger Taster betätigt, OFFSET 22, LENGTH 1
  bool                            : 1; // Padding, OFFSET: 21, LEN: 1
  bool ST2_BET                    : 1; // MSG NAME: ST2_BET - 2-stufiger Taster betätigt, OFFSET 20, LENGTH 1
  bool                            : 1; // Padding, OFFSET: 19, LEN: 1
  bool BH_FUNK_BET                : 1; // MSG NAME: BH_FUNK_BET - Taster Behördenfunk betätigt, OFFSET 18, LENGTH 1
  bool                            : 1; // Padding, OFFSET: 17, LEN: 1
  bool PTS_BET                    : 1; // MSG NAME: PTS_BET - Taster Parktronic betätigt, OFFSET 16, LENGTH 1
  uint8_t                         : 5; // Padding, OFFSET: 27, LEN: 5
  bool LED_STH_DEF                : 1; // MSG NAME: LED_STH_DEF - LEDs für Standheizung defekt, OFFSET 26, LENGTH 1
  bool                            : 1; // Padding, OFFSET: 25, LEN: 1
  bool STHL_BET                   : 1; // MSG NAME: STHL_BET - Schalter Standheizung betätigt, OFFSET 24, LENGTH 1
} __attribute__((__packed__)) UBF_A1_t;


#endif /* W211_CAN_B_H */