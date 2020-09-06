#include "DuckLora.h"
#include "DuckUtils.h"

#ifdef CDPCFG_PIN_LORA_SPI_SCK
#include "SPI.h"
SPIClass _spi;
SPISettings _spiSettings;
CDPCFG_LORA_CLASS lora =
    new Module(CDPCFG_PIN_LORA_CS, CDPCFG_PIN_LORA_DIO0, CDPCFG_PIN_LORA_RST,
               CDPCFG_PIN_LORA_DIO1, _spi, _spiSettings);
#else
CDPCFG_LORA_CLASS lora = new Module(CDPCFG_PIN_LORA_CS, CDPCFG_PIN_LORA_DIO0,
                                    CDPCFG_PIN_LORA_RST, CDPCFG_PIN_LORA_DIO1);
#endif

extern volatile bool getDuckInterrupt();
extern void setDuckInterrupt(bool interrupt);

int DuckLora::setupLoRa(LoraConfigParams config, String deviceId) {
#ifdef CDPCFG_PIN_LORA_SPI_SCK
  log_n("_spi.begin(CDPCFG_PIN_LORA_SPI_SCK, CDPCFG_PIN_LORA_SPI_MISO, "
        "CDPCFG_PIN_LORA_SPI_MOSI, CDPCFG_PIN_LORA_CS)");
  _spi.begin(CDPCFG_PIN_LORA_SPI_SCK, CDPCFG_PIN_LORA_SPI_MISO,
             CDPCFG_PIN_LORA_SPI_MOSI, CDPCFG_PIN_LORA_CS);
  lora = new Module(config.ss, config.di0, config.rst, config.di1, _spi,
                    _spiSettings);
#else
  lora = new Module(config.ss, config.di0, config.rst, config.di1);
#endif

  Serial.println("Starting LoRa......");
  int state = lora.begin();

  if (state == ERR_NONE) {
    Serial.println("LoRa started, Quack!");
  } else {
    return DUCKLORA_ERR_BEGIN;
  }
  // Lora is started, we need to set all the radio parameters, before it can
  // start receiving packets
  if (lora.setFrequency(CDPCFG_RF_LORA_FREQ) == ERR_INVALID_FREQUENCY) {
    Serial.println(
        F("[DuckLora] Selected frequency is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  if (lora.setBandwidth(CDPCFG_RF_LORA_BW) == ERR_INVALID_BANDWIDTH) {
    Serial.println(
        F("[DuckLora] Selected bandwidth is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  if (lora.setSpreadingFactor(CDPCFG_RF_LORA_SF) ==
      ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(
        F("[DuckLora] Selected spreading factor is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  if (lora.setOutputPower(CDPCFG_RF_LORA_TXPOW) == ERR_INVALID_OUTPUT_POWER) {
    Serial.println(
        F("[DuckLora] Selected output power is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  if (lora.setGain(CDPCFG_RF_LORA_GAIN) == ERR_INVALID_GAIN) {
    Serial.println(F("[DuckLora] Selected gain is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  lora.setDio0Action(config.func);

  state = lora.startReceive();

  if (state == ERR_NONE) {
    Serial.println("[DuckLora] Listening for quacks");
  } else {
    Serial.print("[DuckLora] Failed to start receive: ");
    Serial.println(state);
    return DUCKLORA_ERR_RECEIVE;
  }
  _deviceId = deviceId;
  return DUCKLORA_ERR_NONE;
}

void DuckLora::resetTransmission() {
  memset(_transmission, 0x00, CDPCFG_CDP_BUFSIZE);
  _packetIndex = 0;
}

int DuckLora::handlePacket() {
  int pSize = lora.getPacketLength();

  resetTransmission();

  int err = lora.readData(_transmission, pSize);

  if (err != ERR_NONE) {
    Serial.print("[DuckLora] handlePacket failed, code ");
    Serial.println(err);
    return DUCKLORA_ERR_HANDLE_PACKET;
  }

  Serial.print("[DuckLora] handlePacket pSize ");
  Serial.println(pSize);

  Serial.print("[DuckLora] RCV");
  Serial.print(" millis:");
  Serial.print(millis());
  Serial.print(" rssi:");
  Serial.print(lora.getRSSI());
  Serial.print(" snr:");
  Serial.print(lora.getSNR());
  Serial.print(" fe:");
  Serial.print(lora.getFrequencyError());
  Serial.print(" size:");
  Serial.print(pSize);
  Serial.print(" data:");
  Serial.println(duckutils::convertToHex(_transmission, pSize));

  return pSize;
}

String DuckLora::getPacketData(int pSize) {
  String packetData = "";
  if (pSize == 0) {
    Serial.println("getPacketData Packet is empty!");
    return packetData;
  }
  _packetIndex = 0;
  int len = 0;
  bool sId = false;
  bool mId = false;
  bool pLoad = false;
  bool pth = false;
  bool tpc = false;
  String msg = "";
  bool gotLen = false;

  for (int i = 0; i < pSize; i++) {
    if (i > 0 && len == 0) {
      gotLen = false;
      if (sId) {
        _lastPacket.senderId = msg;
        Serial.println("getPacketData User ID: " + _lastPacket.senderId);
        msg = "";
        sId = false;

      } else if (mId) {
        _lastPacket.messageId = msg;
        Serial.println("getPacketData Message ID: " + _lastPacket.messageId);
        msg = "";
        mId = false;

      } else if (pLoad) {
        _lastPacket.payload = msg;
        Serial.println("getPacketData Message: " + _lastPacket.payload);
        msg = "";
        pLoad = false;

      } else if (pth) {
        _lastPacket.path = msg;
        Serial.println("getPacketData Path: " + _lastPacket.path);
        msg = "";
        pth = false;

      } else if (tpc) {
        _lastPacket.topic = msg;
        Serial.println("getPacketData Path: " + _lastPacket.topic);
        msg = "";
        tpc = false;
      }
    }
    if (_transmission[i] == senderId_B) {
      // Serial.println(transmission[1+i]);
      sId = true;
      len = _transmission[i + 1];
      Serial.println("getPacketData senderId_B Len = " + String(len));

    } else if (_transmission[i] == messageId_B) {
      mId = true;
      len = _transmission[i + 1];
      Serial.println("_getPacketData messageId_B Len = " + String(len));

    } else if (_transmission[i] == payload_B) {
      pLoad = true;
      len = _transmission[i + 1];
      Serial.println("_getPacketData payload_B Len = " + String(len));

    } else if (_transmission[i] == path_B) {
      pth = true;
      len = _transmission[i + 1];
      Serial.println("_getPacketData path_B Len = " + String(len));

    } else if (_transmission[i] == topic_B) {
      tpc = true;
      len = _transmission[i + 1];
      Serial.println("_getPacketData topic_B Len = " + String(len));

    } else if (_transmission[i] == ping_B) {
      if (_deviceId != "Det") {
        memset(_transmission, 0x00, CDPCFG_CDP_BUFSIZE);
        _packetIndex = 0;
        couple(iamhere_B, "1");
        startTransmit();
        Serial.println("getPacketData pong sent");
        packetData = "ping";
        return packetData;
      }
      memset(_transmission, 0x00, CDPCFG_CDP_BUFSIZE);
      _packetIndex = 0;
      packetData = "ping";

    } else if (_transmission[i] == iamhere_B) {
      Serial.println("getPacketData pong received");
      memset(_transmission, 0x00, CDPCFG_CDP_BUFSIZE);
      _packetIndex = 0;
      packetData = "pong";
      return packetData;

    } else if (len > 0 && gotLen) {
      msg = msg + String((char)_transmission[i]);
      len--;

    } else {
      gotLen = true;
    }
    _packetIndex++;
  }

  if (len == 0) {
    if (sId) {
      _lastPacket.senderId = msg;
      Serial.println("getPacketData len0 User ID: " + _lastPacket.senderId);
      msg = "";

    } else if (mId) {
      _lastPacket.messageId = msg;
      Serial.println("getPacketData len0 Message ID: " + _lastPacket.messageId);
      msg = "";

    } else if (pLoad) {
      _lastPacket.payload = msg;
      Serial.println("getPacketData len0 Message: " + _lastPacket.payload);
      msg = "";

    } else if (pth) {
      _lastPacket.path = msg;
      Serial.println("getPacketData len0 Path: " + _lastPacket.path);
      msg = "";
    } else if (tpc) {
      _lastPacket.topic = msg;
      Serial.println("getPacketData len0 Topic: " + _lastPacket.topic);
      msg = "";
    }
  }

  return packetData;
}

int DuckLora::sendPayloadStandard(String msg, String topic, String senderId,
                                  String messageId, String path) {

  if (senderId == "") {
    senderId = _deviceId;
  }

  if (topic == "") {
    topic = "status";
  }
  Serial.println("Topic: " + topic);
  if (messageId == "")
    messageId = duckutils::createUuid();
  if (path == "") {
    path = _deviceId;
  } else {
    path = path + "," + _deviceId;
  }

  String total = senderId + messageId + path + msg + topic;
  if (total.length() + 5 > CDPCFG_CDP_BUFSIZE) {
    Serial.println(
        "[DuckLora] Warning: message is too large!"); // TODO: do something
    return DUCKLORA_ERR_HANDLE_PACKET;
  }

  Serial.print("[DuckLora] sendPayloadStandard senderId  ");
  Serial.println(senderId);
  Serial.print("[DuckLora] sendPayloadStandard messageId ");
  Serial.println(messageId);
  Serial.print("[DuckLora] sendPayloadStandard msg       ");
  Serial.println(msg);
  Serial.print("[DuckLora] sendPayloadStandard path      ");
  Serial.println(path);
  Serial.print("[DuckLora] sendPayloadStandard topic     ");
  Serial.println(topic);

  couple(senderId_B, senderId);
  couple(messageId_B, messageId);
  couple(payload_B, msg);
  couple(path_B, path);
  couple(topic_B, topic);

  Serial.print("[DuckLora] sendPayloadStandard packetIndex ");
  Serial.println(_packetIndex);

  startTransmit();
  return DUCKLORA_ERR_NONE;
}

void DuckLora::resetLastPacket() {
  _lastPacket = Packet();
  _lastPacket.topic = "status";
}

Packet DuckLora::getLastPacket() {
  Packet packet = _lastPacket;
  resetLastPacket();
  return packet;
}

// Populates the transmission buffer with the given byteCode and payload
// The format is [ByteCode][PayloadLength][PayloadBytes]
void DuckLora::couple(byte byteCode, String outgoing) {
  int outgoingLen = outgoing.length() + 1;
  byte byteBuffer[outgoingLen];

  outgoing.getBytes(byteBuffer, outgoingLen);

  _transmission[_packetIndex] = byteCode; // add byte code
  _packetIndex++;
  _transmission[_packetIndex] = (byte)outgoingLen; // add payload length
  _packetIndex++;

  for (int i = 0; i < outgoingLen; i++) { // add payload
    _transmission[_packetIndex] = byteBuffer[i];
    _packetIndex++;
  }
}
int DuckLora::startReceive() {
  int state = lora.startReceive();

  if (state != ERR_NONE) {
    Serial.print("[DuckLora] startReceive failed, code ");
    Serial.println(state);
    return DUCKLORA_ERR_RECEIVE;
  }

  return DUCKLORA_ERR_NONE;
}

int DuckLora::startTransmit() {
  bool oldEI = getDuckInterrupt();
  setDuckInterrupt(false);

  // dump send packet stats+data
  Serial.print("[DuckLora] SND");
  Serial.print(" millis:");
  Serial.print(millis());
  Serial.print(" size:");
  Serial.print(_packetIndex);
  Serial.print(" data:");
  Serial.println(duckutils::convertToHex(_transmission, _packetIndex));

  int err = lora.transmit(_transmission, _packetIndex);

  if (err == ERR_NONE) {
    Serial.println("[DuckLora] startTransmit Packet sent");
  } else if (err == ERR_PACKET_TOO_LONG) {
    // the supplied packet was longer than 256 bytes
    Serial.println("[DuckLora] startTransmit too long!");
  } else if (err == ERR_TX_TIMEOUT) {
    Serial.println("[DuckLora] startTransmit timeout!");
  } else {
    Serial.print(F("[DuckLora] startTransmit failed, code "));
    Serial.println(err);
  }

  memset(_transmission, 0x00, CDPCFG_CDP_BUFSIZE);
  _packetIndex = 0;
  // NOTE: Do we need to exit here if transmit failed?

  if (oldEI) {
    setDuckInterrupt(true);
    err = startReceive();
  }

  if (err != ERR_NONE) {
    return DUCKLORA_ERR_RECEIVE;
  }

  return DUCKLORA_ERR_NONE;
}

int DuckLora::getRSSI() { return lora.getRSSI(); }

int DuckLora::ping() {
  couple(ping_B, "0");
  return startTransmit();
}
int DuckLora::standBy() { return lora.standby(); }
