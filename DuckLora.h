#ifndef DUCKLORA_H_
#define DUCKLORA_H_

#include <Arduino.h>
#include <RadioLib.h>

#include "DuckDisplay.h"
#include "LoraPacket.h"
#include "cdpcfg.h"

#define DUCKLORA_ERR_NONE 0
#define DUCKLORA_ERR_BEGIN -1
#define DUCKLORA_ERR_SETUP -2
#define DUCKLORA_ERR_RECEIVE -3

#define DUCKLORA_ERR_HANDLE_PACKET -100
#define DUCKLORA_ERR_MSG_TOO_LARGE -101
const byte ping_B = 0xF4;
const byte senderId_B = 0xF5;
const byte topic_B = 0xE3;
const byte messageId_B = 0xF6;
const byte payload_B = 0xF7;
const byte iamhere_B = 0xF8;
const byte path_B = 0xF3;

typedef struct {
  long band;
  int ss;
  int rst;
  int di0;
  int di1;
  int txPower;
  void (*func)(void); // interrupt service routine function when di0 activates
} LoraConfigParams;

class DuckLora {
public:
  DuckLora(){};

  int setupLoRa(LoraConfigParams config, String deviceId);
  int handlePacket();
  String getPacketData(int pSize);
  int sendPayloadStandard(String msg = "", String topic = "",
                                  String senderId = "", String messageId = "",
                                  String path = "");
  Packet getLastPacket();
  void couple(byte byteCode, String outgoing);
  bool idInPath(String path);

  bool loraPacketReceived();
  int startReceive();
  int startTransmit();
  int getRSSI();
  int getPacketIndex() {return _packetIndex;}
  byte* getTransmission() {return _transmission;}
  int ping();
  int standBy();

private:
  byte _transmission[CDPCFG_CDP_BUFSIZE];
  int _packetIndex = 0;
  Packet _lastPacket;
  String _deviceId = "";
  int _availableBytes = 0;
  int _packetSize = 0;
  void resetTransmission();
  void resetLastPacket();
};

#endif
