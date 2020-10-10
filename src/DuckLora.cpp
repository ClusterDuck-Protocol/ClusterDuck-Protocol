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

int DuckLora::setupLoRa(LoraConfigParams config) {
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

  int state = lora.begin(config.band);
  
  if (state != ERR_NONE) {
    Serial.print("[DuckLora] Failed to start. state = ");
    Serial.println(state);
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
  return DUCK_ERR_NONE;
}

int DuckLora::getReceivedData(std::vector<byte>* packetBytes) {

  int pSize = 0;
  int err = DUCK_ERR_NONE;

  pSize = lora.getPacketLength();
  
  if (pSize == 0) {
    Serial.println("[DuckLora] handlePacket rx data length is 0");
    return DUCKLORA_ERR_HANDLE_PACKET;
  }

  packetBytes->resize(pSize);

  err = lora.readData(packetBytes->data(), pSize);
  if (err != ERR_NONE) {
    Serial.print("[DuckLora] handlePacket failed. err: ");
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
  Serial.println("[DuckLora] packet:  " + String(duckutils::convertToHex(packetBytes->data(), packetBytes->size())));
  return err;
}


int DuckLora::sendData(byte* data, int length) {
  return transmitData(data, length);
}

int DuckLora::sendData(DuckPacket packet) {
  return transmitData(packet.getCdpPacketBuffer().data(), packet.getCdpPacketBuffer().size());
}

int DuckLora::sendData(std::vector<byte> data) {
  return transmitData(data.data(), data.size());
}

int DuckLora::startReceive() {
  int state = lora.startReceive();

  if (state != ERR_NONE) {
    Serial.print("[DuckLora] startReceive failed, code ");
    Serial.println(state);
    return DUCKLORA_ERR_RECEIVE;
  }

  return DUCK_ERR_NONE;
}

int DuckLora::getRSSI() { return lora.getRSSI(); }

// TODO: implement this 
int DuckLora::ping() {
  return DUCK_ERR_NOT_SUPPORTED;
}

int DuckLora::standBy() { return lora.standby(); }

int DuckLora::transmitData(byte* data, int length) {

  bool oldEI = duckutils::getDuckInterrupt();
  duckutils::setDuckInterrupt(false);
  long t1 = millis();

  int err = DUCK_ERR_NONE;
  int tx_err = ERR_NONE;
  int rx_err = ERR_NONE;

  Serial.print("[DuckLora] SND data: ");
  Serial.print(duckutils::convertToHex(data, length));
  Serial.println(" :length = "+String(length));

  tx_err = lora.transmit(data, length);

  switch (tx_err) {
    case ERR_NONE:
      Serial.print("[DuckLora] transmitData Packet sent in: ");
      Serial.print((millis() - t1));
      Serial.println("ms");
      break;

    case ERR_PACKET_TOO_LONG:
      // the supplied packet was longer than 256 bytes
      Serial.println("[DuckLora] transmitData too long!");
      err = DUCKLORA_ERR_MSG_TOO_LARGE;
      break;

    case ERR_TX_TIMEOUT:
      Serial.println("[DuckLora] transmitData timeout!");
      err = DUCKLORA_ERR_TIMEOUT;
      break;

    default:
      Serial.print(F("[DuckLora] transmitData failed, code "));
      Serial.println(err);
      err = DUCKLORA_ERR_TRANSMIT;
      break;
  }

  if (err != DUCK_ERR_NONE) {
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