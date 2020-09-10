#include "include/DuckLora.h"
#include "include/DuckUtils.h"

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

DuckLora* DuckLora::instance = NULL;

DuckLora::DuckLora() {}

DuckLora* DuckLora::getInstance() {
  return (instance == NULL) ? new DuckLora : instance;
}

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

  Serial.println("[DuckLora] Starting LoRa......");
  int state = lora.begin();

  if (state == ERR_NONE) {
    Serial.println("[DuckLora] LoRa started, Quack!");
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

  if (state != ERR_NONE) {
    Serial.print("[DuckLora] Failed to start receive: ");
    Serial.println(state);
    return DUCKLORA_ERR_RECEIVE;
  }
  _deviceId = deviceId;
  return DUCKLORA_ERR_NONE;
}

void DuckLora::resetTransmissionBuffer() {
  memset(_transmission, 0x00, CDPCFG_CDP_BUFSIZE);
  _packetIndex = 0;
}

// Reads the packet data from LoRa driver and stores it in our
// transmission buffer.
// Returns the packet length which should be > 0
// A DUCKLORA_ERR_HANDLE_PACKET is returned if there was a failure
// to read the data from the LoRa driver
int DuckLora::handlePacket() {
  int pSize = 0;
  int err = 0;
  pSize = lora.getPacketLength();
  // we get our packet buffer ready to be populated if packet data is present
  resetTransmissionBuffer();
  err = lora.readData(_transmission, pSize);
  if (err != ERR_NONE) {
    Serial.print("[DuckLora] handlePacket failed, code ");
    Serial.println(err);
    return DUCKLORA_ERR_HANDLE_PACKET;
  }
  Serial.print("[DuckLora] RCV");
  Serial.print(" rssi:");
  Serial.print(lora.getRSSI());
  Serial.print(" snr:");
  Serial.print(lora.getSNR());
  Serial.print(" fe:");
  Serial.print(lora.getFrequencyError());
  Serial.print(" size:");
  Serial.println(pSize);
  Serial.print("[DuckLora] DATA:");
  Serial.println(duckutils::convertToHex(_transmission, pSize));

  return pSize;
}

// Checks the packet buffer and returns its content as string
// An empty string is returned if buffer was empty
String DuckLora::getPacketData(int pSize) {
  String packetData = "";
  if (pSize == 0) {
    Serial.println("[DuckLora] getPacketData Packet is empty!");
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
        Serial.println("[DuckLora] getPacketData Sender ID: " +
                       _lastPacket.senderId);
        msg = "";
        sId = false;

      } else if (mId) {
        _lastPacket.messageId = msg;
        Serial.println("[DuckLora] getPacketData Message ID: " +
                       _lastPacket.messageId);
        msg = "";
        mId = false;

      } else if (pLoad) {
        _lastPacket.payload = msg;
        Serial.println("[DuckLora] getPacketData Message: " +
                       _lastPacket.payload);
        msg = "";
        pLoad = false;

      } else if (pth) {
        _lastPacket.path = msg;
        Serial.println("[DuckLora] getPacketData Path: " + _lastPacket.path);
        msg = "";
        pth = false;

      } else if (tpc) {
        _lastPacket.topic = msg;
        Serial.println("[DuckLora] getPacketData Path: " + _lastPacket.topic);
        msg = "";
        tpc = false;
      }
    }
    if (_transmission[i] == senderId_B) {
      sId = true;
      len = _transmission[i + 1];
      Serial.println("[DuckLora] getPacketData senderId_B Len = " +
                     String(len));

    } else if (_transmission[i] == messageId_B) {
      mId = true;
      len = _transmission[i + 1];
      Serial.println("[DuckLora] getPacketData messageId_B Len = " +
                     String(len));

    } else if (_transmission[i] == payload_B) {
      pLoad = true;
      len = _transmission[i + 1];
      Serial.println("[DuckLora] getPacketData payload_B Len = " + String(len));

    } else if (_transmission[i] == path_B) {
      pth = true;
      len = _transmission[i + 1];
      Serial.println("[DuckLora] getPacketData path_B Len = " + String(len));

    } else if (_transmission[i] == topic_B) {
      tpc = true;
      len = _transmission[i + 1];
      Serial.println("[DuckLora] getPacketData topic_B Len = " + String(len));

    } else if (_transmission[i] == ping_B) {
      if (_deviceId != "Det") {
        resetTransmissionBuffer();
        couple(iamhere_B, "1");
        startTransmit();
        Serial.println("[DuckLora] getPacketData pong sent");
        packetData = "ping";
        return packetData;
      }
      resetTransmissionBuffer();
      packetData = "ping";

    } else if (_transmission[i] == iamhere_B) {
      Serial.println("[DuckLora] getPacketData pong received");
      resetTransmissionBuffer();
      packetData = "pong";
      return packetData;

    } else if (len > 0 && gotLen) {
      msg = msg + String((char)_transmission[i]);
      len--;

    } else {
      gotLen = true;
    }
    _packetIndex++;
  } // for loop

  if (len == 0) {
    if (sId) {
      _lastPacket.senderId = msg;
      Serial.println("[DuckLora] getPacketData len0 Sender ID: " +
                     _lastPacket.senderId);
      msg = "";

    } else if (mId) {
      _lastPacket.messageId = msg;
      Serial.println("[DuckLora] getPacketData len0 Message ID: " +
                     _lastPacket.messageId);
      msg = "";

    } else if (pLoad) {
      _lastPacket.payload = msg;
      Serial.println("[DuckLora] getPacketData len0 Message: " +
                     _lastPacket.payload);
      msg = "";

    } else if (pth) {
      _lastPacket.path = msg;
      Serial.println("[DuckLora] getPacketData len0 Path: " + _lastPacket.path);
      msg = "";
    } else if (tpc) {
      _lastPacket.topic = msg;
      Serial.println("[DuckLora] getPacketData len0 Topic: " +
                     _lastPacket.topic);
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
  if (messageId == "") {
    messageId = duckutils::createUuid();
  }
  if (path == "") {
    path = _deviceId;
  } else {
    path = path + "," + _deviceId;
  }

  String total = senderId + messageId + path + msg + topic;
  if (total.length() + 5 > CDPCFG_CDP_BUFSIZE) {
    Serial.println(
      "[DuckLora] Warning: message [" + topic + "] is too large!"); 

    return DUCKLORA_ERR_MSG_TOO_LARGE;
  }

  Serial.print("[DuckLora] sending");
  Serial.print(" SENDER_ID:");
  Serial.print(senderId);
  Serial.print(" * MSG_ID:");
  Serial.print(messageId);
  Serial.print(" * PATH:");
  Serial.print(path);
  Serial.print(" * TOPIC:");
  Serial.print(topic);
  Serial.print(" * MSG:");
  Serial.print(msg);

  couple(senderId_B, senderId);
  couple(messageId_B, messageId);
  couple(payload_B, msg);
  couple(path_B, path);
  couple(topic_B, topic);

  Serial.print(" * LEN:");
  Serial.println(_packetIndex);

  return startTransmit();
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
  
  bool oldEI = duckutils::getDuckInterrupt();
  duckutils::setDuckInterrupt(false);
  long t1 = millis();
  
  int err = DUCKLORA_ERR_NONE;
  int tx_err = ERR_NONE;
  int rx_err = ERR_NONE;

  Serial.print("[DuckLora] SND");
  Serial.print(" data:");
  Serial.println(duckutils::convertToHex(_transmission, _packetIndex));

  tx_err = lora.transmit(_transmission, _packetIndex);

  switch (tx_err) {
    case ERR_NONE:
      Serial.print("[DuckLora] startTransmit Packet sent in: ");
      Serial.print((millis() - t1));
      Serial.println("ms");
      break;

    case ERR_PACKET_TOO_LONG:
      // the supplied packet was longer than 256 bytes
      Serial.println("[DuckLora] startTransmit too long!");
      err = DUCKLORA_ERR_MSG_TOO_LARGE;
      break;

    case ERR_TX_TIMEOUT:
      Serial.println("[DuckLora] startTransmit timeout!");
      err = DUCKLORA_ERR_TIMEOUT;
      break;

    default:
      Serial.print(F("[DuckLora] startTransmit failed, code "));
      Serial.println(err);
      err = DUCKLORA_ERR_TRANSMIT;
      break;
  }

  if (err != DUCKLORA_ERR_NONE) {
    resetTransmissionBuffer();
    return err;
  }

  if (oldEI) {
    duckutils::setDuckInterrupt(true);
    rx_err = startReceive();
    if (rx_err != ERR_NONE) {
      err = DUCKLORA_ERR_RECEIVE;
    }
  }
  
  return err;
}

int DuckLora::getRSSI() { return lora.getRSSI(); }

int DuckLora::ping() {
  couple(ping_B, "0");
  return startTransmit();
}
int DuckLora::standBy() { return lora.standby(); }
