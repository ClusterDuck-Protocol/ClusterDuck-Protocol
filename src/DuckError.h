#ifndef DUCKERROR_H
#define DUCKERROR_H


/// No Error
#define DUCK_ERR_NONE 0
// Feature not supported
#define DUCK_ERR_NOT_SUPPORTED -5000
// Failed to setup device
#define DUCK_ERR_SETUP         -5100
// Device Id is too long
#define DUCK_ERR_ID_TOO_LONG   -5101
#define DUCK_ERR_OTA           -5200

/// Lora module initialization error
#define DUCKLORA_ERR_BEGIN          -1000

/// Lora module configuration error
#define DUCKLORA_ERR_SETUP          -1001
/// Failure to read data from the Lora module
#define DUCKLORA_ERR_RECEIVE        -1002
/// Lora module timeout error
#define DUCKLORA_ERR_TIMEOUT        -1003
// Failed to send data
#define DUCKLORA_ERR_TRANSMIT       -1004
// Failed to handle data received from the Lora module
#define DUCKLORA_ERR_HANDLE_PACKET  -1050
// Attempted to send a message larger than 256 bytes
#define DUCKLORA_ERR_MSG_TOO_LARGE  -1051
// Radio is busy sending data
#define DUCKLORA_ERR_TX_BUSY        -1052

// Wifi network is not availble
#define DUCKWIFI_ERR_NOT_AVAILABLE  -2000
// Wifi is disconnected
#define DUCKWIFI_ERR_DISCONNECTED   -2001
// Wifi generic setup error
#define DUCKWIFI_ERR_AP_CONFIG      -2002
// DNS initialization failed
#define DUCKDNS_ERR_STARTING        -3000


#define DUCKPACKET_ERR_SIZE_INVALID  -4000
#define DUCKPACKET_ERR_TOPIC_INVALID -4001
#define DUCKPACKET_ERR_MAX_HOPS      -4002

#define DUCK_INTERNET_ERR_SETUP      -6000
#define DUCK_INTERNET_ERR_SSID       -6001
#define DUCK_INTERNET_ERR_CONNECT    -6002

#endif