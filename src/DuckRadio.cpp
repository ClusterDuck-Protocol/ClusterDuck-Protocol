#include "include/DuckRadio.h"

#if !defined(CDPCFG_HELTEC_CUBE_CELL)
#include "include/DuckUtils.h"
#include <RadioLib.h>

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

DuckRadio* DuckRadio::instance = NULL;

DuckRadio::DuckRadio() {}

DuckRadio* DuckRadio::getInstance() {
  return (instance == NULL) ? new DuckRadio : instance;
}

int DuckRadio::setupRadio(LoraConfigParams config) {
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
    Serial.print("[DuckRadio] Failed to start. state = ");
    Serial.println(state);
    return DUCKLORA_ERR_BEGIN;
  }
  
  // Lora is started, we need to set all the radio parameters, before it can
  // start receiving packets
  if (lora.setFrequency(CDPCFG_RF_LORA_FREQ) == ERR_INVALID_FREQUENCY) {
    Serial.println(
        F("[DuckRadio] Selected frequency is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  if (lora.setBandwidth(CDPCFG_RF_LORA_BW) == ERR_INVALID_BANDWIDTH) {
    Serial.println(
        F("[DuckRadio] Selected bandwidth is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  if (lora.setSpreadingFactor(CDPCFG_RF_LORA_SF) ==
      ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(
        F("[DuckRadio] Selected spreading factor is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  if (lora.setOutputPower(CDPCFG_RF_LORA_TXPOW) == ERR_INVALID_OUTPUT_POWER) {
    Serial.println(
        F("[DuckRadio] Selected output power is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  if (lora.setGain(CDPCFG_RF_LORA_GAIN) == ERR_INVALID_GAIN) {
    Serial.println(F("[DuckRadio] Selected gain is invalid for this module!"));
    return DUCKLORA_ERR_SETUP;
  }

  lora.setDio0Action(config.func);
  lora.setSyncWord(0x12);
  
  state = lora.startReceive();

  if (state != ERR_NONE) {
    Serial.print("[DuckRadio] Failed to start receive: ");
    Serial.println(state);
    return DUCKLORA_ERR_RECEIVE;
  }
  return DUCK_ERR_NONE;
}

int DuckRadio::getReceivedData(std::vector<byte>* packetBytes) {

  int pSize = 0;
  int err = DUCK_ERR_NONE;

  pSize = lora.getPacketLength();
  
  if (pSize == 0) {
    Serial.println("[DuckRadio] handlePacket rx data length is 0");
    return DUCKLORA_ERR_HANDLE_PACKET;
  }

  packetBytes->resize(pSize);

  err = lora.readData(packetBytes->data(), pSize);
  if (err != ERR_NONE) {
    Serial.print("[DuckRadio] handlePacket failed. err: ");
    Serial.println(err);
    return DUCKLORA_ERR_HANDLE_PACKET;
  }

  Serial.print("[DuckRadio] RCV");
  Serial.print(" rssi:");
  Serial.print(lora.getRSSI());
  Serial.print(" snr:");
  Serial.print(lora.getSNR());
  Serial.print(" fe:");
  Serial.print(lora.getFrequencyError());
  Serial.print(" size:");
  Serial.println(pSize);
  Serial.println("[DuckRadio] packet:  " + String(duckutils::convertToHex(packetBytes->data(), packetBytes->size())));
  return err;
}


int DuckRadio::sendData(byte* data, int length) {
  return startTransmitData(data, length);
}

int DuckRadio::sendData(DuckPacket* packet) {
  return startTransmitData(packet->getCdpPacketBuffer().data(), packet->getCdpPacketBuffer().size());
}

int DuckRadio::sendData(std::vector<byte> data) {
  return startTransmitData(data.data(), data.size());
}

int DuckRadio::startReceive() {
  int state = lora.startReceive();

  if (state != ERR_NONE) {
    Serial.print("[DuckRadio] startReceive failed, code ");
    Serial.println(state);
    return DUCKLORA_ERR_RECEIVE;
  }

  return DUCK_ERR_NONE;
}

int DuckRadio::getRSSI() { return lora.getRSSI(); }

// TODO: implement this 
int DuckRadio::ping() {
  return DUCK_ERR_NOT_SUPPORTED;
}

int DuckRadio::standBy() { return lora.standby(); }

void DuckRadio::processRadioIrq() {}

int DuckRadio::startTransmitData(byte* data, int length) {

  bool oldEI = duckutils::getDuckInterrupt();
  duckutils::setDuckInterrupt(false);
  long t1 = millis();

  int err = DUCK_ERR_NONE;
  int tx_err = ERR_NONE;
  int rx_err = ERR_NONE;

  Serial.print("[DuckRadio] SND data: ");
  Serial.print(duckutils::convertToHex(data, length));
  Serial.println(" :length = "+String(length));

  tx_err = lora.transmit(data, length);

  switch (tx_err) {
    case ERR_NONE:
      Serial.print("[DuckRadio] startTransmitData Packet sent in: ");
      Serial.print((millis() - t1));
      Serial.println("ms");
      break;

    case ERR_PACKET_TOO_LONG:
      // the supplied packet was longer than 256 bytes
      Serial.println("[DuckRadio] startTransmitData too long!");
      err = DUCKLORA_ERR_MSG_TOO_LARGE;
      break;

    case ERR_TX_TIMEOUT:
      Serial.println("[DuckRadio] startTransmitData timeout!");
      err = DUCKLORA_ERR_TIMEOUT;
      break;

    default:
      Serial.print(F("[DuckRadio] startTransmitData failed, code "));
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
#endif