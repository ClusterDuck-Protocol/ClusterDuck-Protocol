#ifndef LORAPACKET_H_
#define LORAPACKET_H_

#include <Arduino.h>
#include <WString.h>

/**
 * @brief Internal cluster duck LoRa message structure
 * 
 */
typedef struct {
  /// Unique id of the duck sending the packet
  String senderId;
  /// The message topic. Some topics are preset. For example "status", "health"
  String topic;
  /// Unique id representing the message
  String messageId;
  /// Message payload
  String payload;
  /// Comma separated list of duck device ids that have seen this packet
  String path;        
} Packet;

#endif