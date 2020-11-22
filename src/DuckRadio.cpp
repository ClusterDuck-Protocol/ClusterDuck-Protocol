#include "include/DuckRadio.h"
#include "MemoryFree.h"
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

  // TODO: Display should be setup outside the radio setup
  display->setupDisplay(DuckType::MAMA, "mama");

  int rc = lora.begin();

  if (rc != ERR_NONE) {
    logerr("ERROR  initializing LoRa driver. state = ");
    logerr(rc);
    return DUCKLORA_ERR_BEGIN;
  }

  // Lora is started, we need to set all the radio parameters, before it can
  // start receiving packets
  rc = lora.setFrequency(CDPCFG_RF_LORA_FREQ);
  if (rc == ERR_INVALID_FREQUENCY) {
    logerr("ERROR  frequency is invalid");
    return DUCKLORA_ERR_SETUP;
  }

  rc = lora.setBandwidth(CDPCFG_RF_LORA_BW);
  if (rc == ERR_INVALID_BANDWIDTH) {
    logerr("ERROR  bandwidth is invalid");
    return DUCKLORA_ERR_SETUP;
  }

  rc = lora.setSpreadingFactor(CDPCFG_RF_LORA_SF);
  if (rc == ERR_INVALID_SPREADING_FACTOR) {
    logerr("ERROR  spreading factor is invalid");
    return DUCKLORA_ERR_SETUP;
  }

  rc = lora.setOutputPower(CDPCFG_RF_LORA_TXPOW);
  if (rc == ERR_INVALID_OUTPUT_POWER) {
    logerr("ERROR  output power is invalid");
    return DUCKLORA_ERR_SETUP;
  }

  rc = lora.setGain(CDPCFG_RF_LORA_GAIN);
  if (rc == ERR_INVALID_GAIN) {
    logerr("ERROR  gain is invalid");
    return DUCKLORA_ERR_SETUP;
  }

  // set the interrupt handler to execute when packet tx or rx is done.
  lora.setDio0Action(config.func);
  // set sync word to private network
  if (lora.setSyncWord(0x12) != ERR_NONE) {
    logerr("ERROR  sync word is invalid");
    return DUCKLORA_ERR_SETUP;
  }

  rc = lora.startReceive();

  if (rc != ERR_NONE) {
    logerr("ERROR Failed to start receive");
    return DUCKLORA_ERR_RECEIVE;
  }
  return DUCK_ERR_NONE;
}

int DuckRadio::readReceivedData(std::vector<byte>* packetBytes) {

  int packet_length = 0;
  int err = DUCK_ERR_NONE;

  packet_length = lora.getPacketLength();

  if (packet_length < MIN_PACKET_LENGTH) {
    logerr("ERROR  handlePacket rx data size invalid: " +
           String(packet_length));
    return DUCKLORA_ERR_HANDLE_PACKET;
  }

  loginfo("readReceivedData() - packet length returns: " +
          String(packet_length));

  packetBytes->resize(packet_length);
  err = lora.readData(packetBytes->data(), packet_length);
  loginfo("readReceivedData() - lora.readData returns: " + String(err));

  if (err != ERR_NONE) {
    logerr("ERROR  readReceivedData failed. err: " + String(err));
    return DUCKLORA_ERR_HANDLE_PACKET;
  }

  loginfo("readReceivedData: checking path offset integrity");

  // Do some sanity checks on the received packet here before we continue
  // further RadioLib v4.0.5 has a bug where corrupt packets are still returned
  // to the app despite CRC check being enabled in the radio by both sender and
  // receiver.

  byte* data = packetBytes->data();
  int path_pos = data[PATH_OFFSET_POS];
  if (path_pos >= packetBytes->size()) {
    logerr("ERROR path offset out of bound. Data is probably corrupted.");
    return DUCKLORA_ERR_HANDLE_PACKET;
  }

  loginfo("readReceivedData: checking data section CRC");

  std::vector<byte> data_section;
  data_section.insert(data_section.end(), &data[DATA_POS], &data[path_pos]);
  uint32_t packet_data_crc = duckutils::toUnit32(&data[DATA_CRC_POS]);
  uint32_t computed_data_crc =
      CRC32::calculate(data_section.data(), data_section.size());

  if (computed_data_crc != packet_data_crc) {
    logerr("ERROR data crc mismatch: received: " + String(packet_data_crc) +
           " calculated:" + String(computed_data_crc));
    return DUCKLORA_ERR_HANDLE_PACKET;
  }
  // we have a good packet
  loginfo("RX: rssi: " + String(lora.getRSSI()) +
          " snr: " + String(lora.getSNR()) +
          " fe: " + String(lora.getFrequencyError(true)) +
          " size: " + String(packet_length));

  return err;
}

int DuckRadio::sendData(byte* data, int length) {
  // display->log("sendData...");
  return startTransmitData(data, length);
}

int DuckRadio::relayPacket(DuckPacket* packet) {
  // display->log("relayPacket...");
  return startTransmitData(packet->getBuffer().data(),
                           packet->getBuffer().size());
}

int DuckRadio::sendData(std::vector<byte> data) {
  // display->log("sendData...");
  return startTransmitData(data.data(), data.size());
}

int DuckRadio::startReceive() {
  int state = lora.startReceive();

  if (state != ERR_NONE) {
    logerr("ERROR startReceive failed, code " + String(state));
    return DUCKLORA_ERR_RECEIVE;
  }

  return DUCK_ERR_NONE;
}

int DuckRadio::getRSSI() { return lora.getRSSI(); }

// TODO: implement this
int DuckRadio::ping() { return DUCK_ERR_NOT_SUPPORTED; }

int DuckRadio::standBy() { return lora.standby(); }

int DuckRadio::sleep() { return lora.sleep(); }

void DuckRadio::processRadioIrq() {}

int DuckRadio::startTransmitData(byte* data, int length) {

  bool interruptWasEnabled = duckutils::isInterruptEnabled();
  // we are ready to transmit so radio cannot be interrupted
  duckutils::setInterrupt(false);

  int err = DUCK_ERR_NONE;
  int tx_err = ERR_NONE;
  int rx_err = ERR_NONE;
  loginfo("TX data");
  logdbg(" -> " + duckutils::convertToHex(data, length));
  logdbg(" -> length: " + String(length));

  long t1 = millis();
  // this is going to wait for transmission to complete or to timeout
  // when transmit is complete, the Di0 interrupt will be triggered
  tx_err = lora.transmit(data, length);
  switch (tx_err) {
    case ERR_NONE:
      loginfo("TX data done in : " + String((millis() - t1)) + "ms");
      // display->log(">> txdone");
      break;

    case ERR_PACKET_TOO_LONG:
      // the supplied packet was longer than 256 bytes
      logerr("ERROR startTransmitData too long!");
      err = DUCKLORA_ERR_MSG_TOO_LARGE;
      break;

    case ERR_TX_TIMEOUT:
      logerr("ERROR startTransmitData timeout!");
      display->log("tx-tmout");
      err = DUCKLORA_ERR_TIMEOUT;
      break;

    default:
      logerr("ERROR startTransmitData failed, err: " + String(tx_err));
      display->log("tx-fail");
      err = DUCKLORA_ERR_TRANSMIT;
      break;
  }

  if (interruptWasEnabled) {
    duckutils::setInterrupt(true);
    // we can set the radio back to receive mode
    // di0 interrupt will trigger again and set a receive flag
    // if a packet is received. See void Duck::onRadioRxTxDone(void)
    rx_err = startReceive();
    if (rx_err != ERR_NONE) {
      display->log("strt-rx-fail");
      err = DUCKLORA_ERR_RECEIVE;
    }
  }
  return err;
}
#endif