#ifndef LORAPACKET_H_
#define LORAPACKET_H_

#include <Arduino.h>
#include <WString.h>

/**
 * @brief A duck LoRa Packet
 * 
 */
typedef struct {
  String senderId;
  String topic;
  String messageId;
  String payload;
  String path;
} Packet;

#endif