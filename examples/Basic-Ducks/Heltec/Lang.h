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

  #define TXT_BLE_ADV_FAIL           "BLE ERROR\nCANNOT ADVERTISE"
  #define TXT_PHONE_GPS_NO_SIGNAL_2L "PHONE GPS\nNO SIGNAL"
  #define TXT_GPS_SENT_OK            "GPS SENT OK!"
  #define TXT_GPS_SEND_FAIL          "GPS SEND FAILED!"
  #define TXT_SRC_PHONE              "SRC:PHONE"
  #define TXT_BT_CONNECTED           "BLUETOOTH\nCONNECTED!"
  #define TXT_BT_DISCONNECTED        "BLUETOOTH\nDISCONNECTED"
  #define TXT_USB_CONNECTED          "USB SERIAL\nCONNECTED!"
  #define TXT_USB_DISCONNECTED       "USB SERIAL\nDISCONNECTED"
  #define TXT_SOS_ACK_DISPLAY        "SOS RECEIVED!\nHELP IS BEING\nSENT"
  #define TXT_SENDING_SOS_2L         "SENDING\nEMERGENCY SIGNAL..."
  #define TXT_REQ_GPS_FROM_PHONE_2L  "REQUESTING GPS\nFROM PHONE..."
  #define TXT_SOS_CANCELLED          "SOS CANCELLED"
  #define TXT_ROGER_SENT             "ROGER SENT!"
  #define TXT_DATETIME_NO_SIGNAL_2L  "DATE/TIME\nNO SIGNAL"
  #define TXT_GPS_MODULE_ACTIVE_2L   "GPS: MODULE ACTIVE\nWAITING FOR SIGNAL..."
  #define TXT_GPS_NO_MODULE          "GPS: NO MODULE"
  #define TXT_SENDING_GPS_DATA       "SENDING GPS DATA"
  #define TXT_REQ_GPS_DATA_FROM_PHONE_2L "REQUESTING GPS DATA\nFROM PHONE..."
  #define TXT_NO_PHONE_NO_GPS_2L     "NO PHONE\nNO GPS DATA"
  #define TXT_SIG_WEAK               "SIG: WEAK ("
  #define TXT_SIG_OK                 "SIG: OK ("
  #define TXT_SIG_STRONG             "SIG: STRONG ("
  #define TXT_SIG_VSTRONG            "SIG: V.STRONG ("
  #define TXT_SEND_OK                "SEND OK"
  #define TXT_SEND_FAIL              "SEND FAILED"
  #define TXT_SIG_NONE               "SIG: NO SIGNAL"
  #define TXT_GPS_SEARCH_FMT         "GPS: SEARCH %uSAT"
  #define TXT_PRESS_BUTTON_ABOVE     "PRESS BUTTON ABOVE"
  #define TXT_2SEC_EMERGENCY         "2 SEC = EMERGENCY"
  #define TXT_HOME_HINT_3L           "PRESS BUTTON ABOVE\nFOR TWO SECONDS FOR\nEMERGENCY SIGNAL"
  #define TXT_HOLD_FOR_SOS           "HOLD FOR SOS"
  #define TXT_SOS_SENT_HINT_3L       "PRESS BUTTON ABOVE\nFOR 2 SECONDS FOR\nEMERGENCY SIGNAL"
  #define TXT_SOS_SENT_GPS_3L        "SENT OK\nEMERGENCY SIGNAL\nWITH GPS!"
  #define TXT_SOS_SENT_NOGPS_3L      "SENT OK\nEMERGENCY SIGNAL\nWITHOUT GPS!"
  #define TXT_SOS_ERR_2L             "ERROR. CANNOT\nSEND EMERGENCY SIGNAL"
  #define TXT_MSG_SENT               "MESSAGE SENT!"

#else  // Bahasa Melayu (default)

  #define TXT_BLE_ADV_FAIL           "RALAT BLE\nTIDAK BOLEH IKLAN"
  #define TXT_PHONE_GPS_NO_SIGNAL_2L "GPS TELEFON\nTIADA ISYARAT"
  #define TXT_GPS_SENT_OK            "BERJAYA HANTAR GPS!"
  #define TXT_GPS_SEND_FAIL          "GAGAL HANTAR GPS!"
  #define TXT_SRC_PHONE              "SRC:TELEFON"
  #define TXT_BT_CONNECTED           "BLUETOOTH\nTERSAMBUNG!"
  #define TXT_BT_DISCONNECTED        "BLUETOOTH\nTERPUTUS"
  #define TXT_USB_CONNECTED          "USB BERSIRI\nTERSAMBUNG!"
  #define TXT_USB_DISCONNECTED       "USB BERSIRI\nTERPUTUS"
  #define TXT_SOS_ACK_DISPLAY        "SOS DITERIMA!\nBANTUAN SEDANG\nDIHANTAR"
  #define TXT_SENDING_SOS_2L         "SEDANG HANTAR\nISYARAT KECEMASAN..."
  #define TXT_REQ_GPS_FROM_PHONE_2L  "MEMINTA GPS\nDARIPADA TELEFON..."
  #define TXT_SOS_CANCELLED          "SOS DIBATALKAN"
  #define TXT_ROGER_SENT             "ROGER DIHANTAR!"
  #define TXT_DATETIME_NO_SIGNAL_2L  "TARIKH/MASA\nTIADA ISYARAT"
  #define TXT_GPS_MODULE_ACTIVE_2L   "GPS: MODUL AKTIF\nMENUNGGU ISYARAT..."
  #define TXT_GPS_NO_MODULE          "GPS: TIADA MODUL"
  #define TXT_SENDING_GPS_DATA       "MENGHANTAR DATA GPS"
  #define TXT_REQ_GPS_DATA_FROM_PHONE_2L "MEMINTA DATA GPS\nDARIPADA TELEFON..."
  #define TXT_NO_PHONE_NO_GPS_2L     "TIADA TELEFON\nTIADA DATA GPS"
  #define TXT_SIG_WEAK               "SIG: LEMAH ("
  #define TXT_SIG_OK                 "SIG: CUKUP ("
  #define TXT_SIG_STRONG             "SIG: KUAT ("
  #define TXT_SIG_VSTRONG            "SIG: SG.KUAT ("
  #define TXT_SEND_OK                "BERJAYA HANTAR"
  #define TXT_SEND_FAIL              "GAGAL HANTAR"
  #define TXT_SIG_NONE               "SIG: TIADA ISYARAT"
  #define TXT_GPS_SEARCH_FMT         "GPS: CARI %uSAT"
  #define TXT_PRESS_BUTTON_ABOVE     "TEKAN BUTANG ATAS"
  #define TXT_2SEC_EMERGENCY         "2 SAAT = KECEMASAN"
  #define TXT_HOME_HINT_3L           "TEKAN BUTANG ATAS\nSELAMA DUA SAAT UTK\nISYARAT KECEMASAN"
  #define TXT_HOLD_FOR_SOS           "TAHAN UNTUK SOS"
  #define TXT_SOS_SENT_HINT_3L       "TEKAN BUTANG ATAS\nSELAMA 2 SAAT UTK\nISYARAT KECEMASAN"
  #define TXT_SOS_SENT_GPS_3L        "BERJAYA HANTAR\nISYARAT KECEMASAN\nDENGAN GPS!"
  #define TXT_SOS_SENT_NOGPS_3L      "BERJAYA HANTAR\nISYARAT KECEMASAN\nTANPA GPS!"
  #define TXT_SOS_ERR_2L             "RALAT. TIDAK BOLEH\nHANTAR ISYARAT KECEMASAN"
  #define TXT_MSG_SENT               "MESEJ TELAH DIHANTAR!"

#endif
