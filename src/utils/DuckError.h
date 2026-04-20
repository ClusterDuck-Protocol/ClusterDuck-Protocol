#ifndef DUCKERROR_H
#define DUCKERROR_H

/// Duck error codes enum
enum DuckError : int {
    /// No Error
    DUCK_ERR_NONE = 0,

    /// Feature not supported
    DUCK_ERR_NOT_SUPPORTED = -5000,
    /// Failed to setup device
    DUCK_ERR_SETUP = -5100,
    /// Invalid argument
    DUCK_ERR_INVALID_ARGUMENT = -5101,
    /// Device Id is too long
    DUCK_ERR_ID_TOO_LONG = -5101,

    /// Lora module initialization error
    DUCKLORA_ERR_BEGIN = -1000,
    /// Lora module configuration error
    DUCKLORA_ERR_SETUP = -1001,
    /// Failure to read data from the Lora module
    DUCKLORA_ERR_RECEIVE = -1002,
    /// Lora module timeout error
    DUCKLORA_ERR_TIMEOUT = -1003,
    /// Failed to send data
    DUCKLORA_ERR_TRANSMIT = -1004,
    /// Failed to handle data received from the Lora module
    DUCKLORA_ERR_HANDLE_PACKET = -1050,
    /// Attempted to send a message larger than 256 bytes
    DUCKLORA_ERR_MSG_TOO_LARGE = -1051,
    /// Radio is busy sending data
    DUCKLORA_ERR_TX_BUSY = -1052,
    /// Invalid channel
    DUCKLORA_ERR_INVALID_CHANNEL = -1053,
    /// Radio not initialized
    DUCKLORA_ERR_NOT_INITIALIZED = -1054,
    /// Lora standby error
    DUCKLORA_ERR_STANDBY = -1055,
    /// Lora sleep error
    DUCKLORA_ERR_SLEEP = -1056,

    /// Wifi network is not available
    DUCKWIFI_ERR_NOT_AVAILABLE = -2000,
    /// Wifi is disconnected
    DUCKWIFI_ERR_DISCONNECTED = -2001,
    /// Wifi generic setup error
    DUCKWIFI_ERR_AP_CONFIG = -2002,
    /// DNS initialization failed
    DUCKDNS_ERR_STARTING = -3000,

    /// Packet size invalid
    DUCKPACKET_ERR_SIZE_INVALID = -4000,
    /// Packet topic invalid
    DUCKPACKET_ERR_TOPIC_INVALID = -4001,
    /// Packet max hops exceeded
    DUCKPACKET_ERR_MAX_HOPS = -4002,

    /// Internet setup error
    DUCK_INTERNET_ERR_SETUP = -6000,
    /// Internet SSID error
    DUCK_INTERNET_ERR_SSID = -6001,
    /// Internet connection error
    DUCK_INTERNET_ERR_CONNECT = -6002,

    /// EEPROM initialization error
    DUCK_ERR_EEPROM_INIT = -7000,
    /// EEPROM write error
    DUCK_ERR_EEPROM_WRITE = -7001,
    /// EEPROM read error
    DUCK_ERR_EEPROM_READ = -7002
};

/// Get human-readable error string for a given error code
/// @param error The error code
/// @return A string describing the error
inline const char* getDuckErrorString(int error) {
    switch(error) {
        case DUCK_ERR_NONE:
            return "No Error";
        case DUCK_ERR_NOT_SUPPORTED:
            return "Feature not supported";
        case DUCK_ERR_SETUP:
            return "Failed to setup device";
        case DUCK_ERR_INVALID_ARGUMENT:
            return "Invalid argument";
        case DUCK_ERR_ID_TOO_LONG:
            return "Device Id is too long";

        case DUCKLORA_ERR_BEGIN:
            return "Lora module initialization error";
        case DUCKLORA_ERR_SETUP:
            return "Lora module configuration error";
        case DUCKLORA_ERR_RECEIVE:
            return "Failure to read data from the Lora module";
        case DUCKLORA_ERR_TIMEOUT:
            return "Lora module timeout error";
        case DUCKLORA_ERR_TRANSMIT:
            return "Failed to send data";
        case DUCKLORA_ERR_HANDLE_PACKET:
            return "Failed to handle data received from the Lora module";
        case DUCKLORA_ERR_MSG_TOO_LARGE:
            return "Attempted to send a message larger than 256 bytes";
        case DUCKLORA_ERR_TX_BUSY:
            return "Radio is busy sending data";
        case DUCKLORA_ERR_INVALID_CHANNEL:
            return "Invalid channel";
        case DUCKLORA_ERR_NOT_INITIALIZED:
            return "Radio not initialized";
        case DUCKLORA_ERR_STANDBY:
            return "Lora standby error";
        case DUCKLORA_ERR_SLEEP:
            return "Lora sleep error";

        case DUCKWIFI_ERR_NOT_AVAILABLE:
            return "Wifi network is not available";
        case DUCKWIFI_ERR_DISCONNECTED:
            return "Wifi is disconnected";
        case DUCKWIFI_ERR_AP_CONFIG:
            return "Wifi generic setup error";
        case DUCKDNS_ERR_STARTING:
            return "DNS initialization failed";

        case DUCKPACKET_ERR_SIZE_INVALID:
            return "Packet size invalid";
        case DUCKPACKET_ERR_TOPIC_INVALID:
            return "Packet topic invalid";
        case DUCKPACKET_ERR_MAX_HOPS:
            return "Packet max hops exceeded";

        case DUCK_INTERNET_ERR_SETUP:
            return "Internet setup error";
        case DUCK_INTERNET_ERR_SSID:
            return "Internet SSID error";
        case DUCK_INTERNET_ERR_CONNECT:
            return "Internet connection error";

        case DUCK_ERR_EEPROM_INIT:
            return "EEPROM initialization error";
        case DUCK_ERR_EEPROM_WRITE:
            return "EEPROM write error";
        case DUCK_ERR_EEPROM_READ:
            return "EEPROM read error";

        default:
            return "Unknown error";
    }
}

#endif