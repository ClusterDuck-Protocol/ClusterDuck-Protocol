#pragma once

// ── Display language selection ─────────────────────────────────────────────
// All on-screen (OLED) text in this sketch defaults to Bahasa Melayu. To
// compile with English UI text instead, either:
//   1) uncomment the #define below, or
//   2) add a build flag in platformio.ini:  -DDUCK_LANG_EN
//
// NOTE: This only affects text shown on the display. It does NOT change any
// wire-protocol / mesh keywords (e.g. the "SOS DITERIMA" content check and
// "CDK:SOS_ACK,TEXT:SOS DITERIMA" broadcast), which are a fixed contract with
// the external OpenDMS system and must remain unchanged regardless of the
// sketch's display language.

// #define DUCK_LANG_EN

#if defined(DUCK_LANG_EN)

  #define TXT_PHONE_GPS              "PHONE GPS"
  #define TXT_NO_SIGNAL              "NO SIGNAL"
  #define TXT_GPS_SENT_OK            "GPS SENT OK!"
  #define TXT_GPS_SEND_FAIL          "GPS SEND FAILED!"
  #define TXT_SRC_PHONE              "SRC:PHONE"
  #define TXT_USB_SERIAL             "USB SERIAL"
  #define TXT_CONNECTED_BANG         "CONNECTED!"
  #define TXT_DISCONNECTED           "DISCONNECTED"
  #define TXT_SOS_RECEIVED          "SOS RECEIVED!"
  #define TXT_HELP_BEING             "HELP IS BEING"
  #define TXT_SENT                   "SENT"
  #define TXT_SENDING                "SENDING"
  #define TXT_EMERGENCY_SIGNAL_DOTS  "EMERGENCY SIGNAL..."
  #define TXT_REQUESTING_GPS         "REQUESTING GPS"
  #define TXT_FROM_PHONE_DOTS        "FROM PHONE..."
  #define TXT_CLICK_TO_CANCEL        "(CLICK TO CANCEL)"
  #define TXT_ROGER_SENT             "ROGER SENT!"
  #define TXT_DATE_TIME              "DATE/TIME"
  #define TXT_GPS_MODULE_ACTIVE      "GPS: MODULE ACTIVE"
  #define TXT_GPS_NO_MODULE          "GPS: NO MODULE"
  #define TXT_WAITING_SIGNAL_DOTS    "WAITING FOR SIGNAL..."
  #define TXT_GPS_SEARCH_FMT         "GPS: SEARCH %uSAT"
  #define TXT_SENDING_GPS_DATA       "SENDING GPS DATA"
  #define TXT_REQUESTING_GPS_DATA    "REQUESTING GPS DATA"
  #define TXT_NO_PHONE               "NO PHONE"
  #define TXT_NO_GPS_DATA            "NO GPS DATA"
  #define TXT_SIG_WEAK               "SIG: WEAK ("
  #define TXT_SIG_OK                 "SIG: OK ("
  #define TXT_SIG_STRONG             "SIG: STRONG ("
  #define TXT_SIG_VSTRONG            "SIG: V.STRONG ("
  #define TXT_SEND_OK                "SEND OK"
  #define TXT_SEND_FAIL              "SEND FAILED"
  #define TXT_SIG_NONE               "SIG: NO SIGNAL"
  #define TXT_PRESS_BUTTON_ABOVE     "PRESS BUTTON ABOVE"
  #define TXT_2SEC_EMERGENCY         "2 SEC = EMERGENCY"
  #define TXT_HOLD_FOR_SOS           "HOLD FOR SOS"
  #define TXT_SOS_SENT_HINT_3L       "PRESS BUTTON ABOVE\nFOR 2 SECONDS FOR\nEMERGENCY SIGNAL"
  #define TXT_EMERGENCY_SIGNAL       "EMERGENCY SIGNAL"
  #define TXT_WITH_GPS               "WITH GPS!"
  #define TXT_WITHOUT_GPS            "WITHOUT GPS!"
  #define TXT_SOS_ERR_CANNOT         "ERROR. CANNOT"
  #define TXT_SEND_SIGNAL            "SEND SIGNAL"
  #define TXT_EMERGENCY              "EMERGENCY"
  #define TXT_MSG_SENT               "MESSAGE SENT!"

#else  // Bahasa Melayu (default)

  #define TXT_PHONE_GPS              "GPS TELEFON"
  #define TXT_NO_SIGNAL              "TIADA ISYARAT"
  #define TXT_GPS_SENT_OK            "BERJAYA HANTAR GPS!"
  #define TXT_GPS_SEND_FAIL          "GAGAL HANTAR GPS!"
  #define TXT_SRC_PHONE              "SRC:TELEFON"
  #define TXT_USB_SERIAL             "USB BERSIRI"
  #define TXT_CONNECTED_BANG         "TERSAMBUNG!"
  #define TXT_DISCONNECTED           "TERPUTUS"
  #define TXT_SOS_RECEIVED          "SOS DITERIMA!"
  #define TXT_HELP_BEING             "BANTUAN SEDANG"
  #define TXT_SENT                   "DIHANTAR"
  #define TXT_SENDING                "SEDANG HANTAR"
  #define TXT_EMERGENCY_SIGNAL_DOTS  "ISYARAT KECEMASAN..."
  #define TXT_REQUESTING_GPS         "MEMINTA GPS"
  #define TXT_FROM_PHONE_DOTS        "DARIPADA TELEFON..."
  #define TXT_CLICK_TO_CANCEL        "(KLIK UNTUK BATAL)"
  #define TXT_ROGER_SENT             "ROGER DIHANTAR!"
  #define TXT_DATE_TIME              "TARIKH/MASA"
  #define TXT_GPS_MODULE_ACTIVE      "GPS: MODUL AKTIF"
  #define TXT_GPS_NO_MODULE          "GPS: TIADA MODUL"
  #define TXT_WAITING_SIGNAL_DOTS    "MENUNGGU ISYARAT..."
  #define TXT_GPS_SEARCH_FMT         "GPS: CARI %uSAT"
  #define TXT_SENDING_GPS_DATA       "MENGHANTAR DATA GPS"
  #define TXT_REQUESTING_GPS_DATA    "MEMINTA DATA GPS"
  #define TXT_NO_PHONE               "TIADA TELEFON"
  #define TXT_NO_GPS_DATA            "TIADA DATA GPS"
  #define TXT_SIG_WEAK               "SIG: LEMAH ("
  #define TXT_SIG_OK                 "SIG: CUKUP ("
  #define TXT_SIG_STRONG             "SIG: KUAT ("
  #define TXT_SIG_VSTRONG            "SIG: SG.KUAT ("
  #define TXT_SEND_OK                "BERJAYA HANTAR"
  #define TXT_SEND_FAIL              "GAGAL HANTAR"
  #define TXT_SIG_NONE               "SIG: TIADA ISYARAT"
  #define TXT_PRESS_BUTTON_ABOVE     "TEKAN BUTANG ATAS"
  #define TXT_2SEC_EMERGENCY         "2 SAAT = KECEMASAN"
  #define TXT_HOLD_FOR_SOS           "TAHAN UNTUK SOS"
  #define TXT_SOS_SENT_HINT_3L       "TEKAN BUTANG ATAS\nSELAMA 2 SAAT UTK\nISYARAT KECEMASAN"
  #define TXT_EMERGENCY_SIGNAL       "ISYARAT KECEMASAN"
  #define TXT_WITH_GPS               "DENGAN GPS!"
  #define TXT_WITHOUT_GPS            "TANPA GPS!"
  #define TXT_SOS_ERR_CANNOT         "RALAT. TIDAK BOLEH"
  #define TXT_SEND_SIGNAL            "HANTAR ISYARAT"
  #define TXT_EMERGENCY              "KECEMASAN"
  #define TXT_MSG_SENT               "MESEJ TELAH DIHANTAR!"

#endif
