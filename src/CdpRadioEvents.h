#ifndef CDP_RADIO_EVENTS_
#define CDP_RADIO_EVENTS_

#include "Arduino.h"

typedef struct {
  /**
   * @brief  Tx Done callback prototype.
   */
  void (*TxDone)(void);
  
  /**
   * @brief  Tx Timeout callback prototype.
   */
  void (*TxTimeout)(void);
  
  /**
   * @brief Rx Done callback prototype.
   *
   * @param [IN] payload Received buffer pointer
   * @param [IN] size    Received buffer size
   * @param [IN] rssi    RSSI value computed while receiving the frame [dBm]
   * @param [IN] snr     Raw SNR value given by the radio hardware
   *                     LoRa SNR value qin dB
   */
  void (*RxDone)(byte* payload, uint16_t size, int16_t rssi, int8_t snr);
  
  /**
   * @brief  Rx Timeout callback prototype.
   */
  void (*RxTimeout)(void);
  
  /**
   * @brief Rx Error callback prototype.
   */
  void (*RxError)(void);
  
  /**
   * @brief CAD Done callback prototype.
   *
   * @param [IN] channelDetected    Channel Activity detected during the CAD
   */
  void (*CadDone)(bool channelActivityDetected);
} RadioEvents_t;

#endif