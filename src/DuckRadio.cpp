#include "include/DuckRadio.h"
#ifdef CDPCFG_RADIO_SX126X
#define DUCK_RADIO_IRQ_TIMEOUT RADIOLIB_SX126X_IRQ_TIMEOUT
#define DUCK_RADIO_IRQ_TX_DONE RADIOLIB_SX126X_IRQ_TX_DONE
#define DUCK_RADIO_IRQ_RX_DONE RADIOLIB_SX126X_IRQ_RX_DONE
#define DUCK_RADIO_IRQ_CRC_ERROR RADIOLIB_SX126X_IRQ_CRC_ERR
#define DUCK_RADIO_IRQ_HEADER_ERR RADIOLIB_SX126X_IRQ_HEADER_ERR
#else
#define DUCK_RADIO_IRQ_TIMEOUT RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_TIMEOUT
#define DUCK_RADIO_IRQ_TX_DONE RADIOLIB_SX127X_CLEAR_IRQ_FLAG_TX_DONE
#define DUCK_RADIO_IRQ_RX_DONE RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_DONE
#define DUCK_RADIO_IRQ_CRC_ERROR RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR
#endif

#if !defined(CDPCFG_HELTEC_CUBE_CELL)
#include "include/DuckUtils.h"
#include <RadioLib.h>

#if defined(CDPCFG_PIN_LORA_SCLK) & defined(CDPCFG_RADIO_SX126X)
#include <SPI.h>
SPIClass spi;
SPISettings spiSettings;
CDPCFG_LORA_CLASS lora =
        new Module(CDPCFG_PIN_LORA_CS, CDPCFG_PIN_LORA_DIO1, CDPCFG_PIN_LORA_RST,
                   CDPCFG_PIN_LORA_BUSY, spi);
#elif CDPCFG_RADIO_SX126X
CDPCFG_LORA_CLASS lora =
        new Module(CDPCFG_PIN_LORA_CS, CDPCFG_PIN_LORA_DIO1, CDPCFG_PIN_LORA_RST,
                   CDPCFG_PIN_LORA_BUSY, spi);
#else
CDPCFG_LORA_CLASS lora = new Module(CDPCFG_PIN_LORA_CS, CDPCFG_PIN_LORA_DIO0,
                  CDPCFG_PIN_LORA_RST, CDPCFG_PIN_LORA_DIO1);
#endif

volatile uint16_t DuckRadio::interruptFlags = 0;
volatile bool DuckRadio::receivedFlag = false;

DuckRadio::DuckRadio() {}

int DuckRadio::setupRadio(LoraConfigParams config) {
#if  defined(CDPCFG_PIN_LORA_SCLK) & defined(CDPCFG_RADIO_SX126X)
    log_n("_spi.begin(CDPCFG_PIN_LORA_SPI_SCK, CDPCFG_PIN_LORA_SPI_MISO, "
        "CDPCFG_PIN_LORA_SPI_MOSI, CDPCFG_PIN_LORA_CS)");
  spi.begin(CDPCFG_PIN_LORA_SCLK, CDPCFG_PIN_LORA_MISO,
             CDPCFG_PIN_LORA_MOSI, CDPCFG_PIN_LORA_CS);
#endif

    // TODO: Display should be setup outside the radio setup
    // display->setupDisplay(DuckType::MAMA, "mama");

    int rc = lora.begin();

    if (rc != RADIOLIB_ERR_NONE) {
        logerr("ERROR  initializing LoRa driver. state = ");
        logerr(rc);
        return DUCKLORA_ERR_BEGIN;
    }

    // Lora is started, we need to set all the radio parameters, before it can
    // start receiving packets
    rc = lora.setFrequency(config.band);
    if (rc == RADIOLIB_ERR_INVALID_FREQUENCY) {
        logerr("ERROR  frequency is invalid");
        return DUCKLORA_ERR_SETUP;
    }

    rc = lora.setBandwidth(config.bw);
    if (rc == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        logerr("ERROR  bandwidth is invalid");
        return DUCKLORA_ERR_SETUP;
    }

    rc = lora.setSpreadingFactor(config.sf);
    if (rc == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        logerr("ERROR  spreading factor is invalid");
        return DUCKLORA_ERR_SETUP;
    }

    rc = lora.setOutputPower(config.txPower);
    if (rc == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        logerr("ERROR  output power is invalid");
        return DUCKLORA_ERR_SETUP;
    }
#ifdef CDPCFG_RADIO_SX126X
    // set the interrupt handler to execute when packet tx or rx is done.
    lora.setDio1Action(config.func);
#else
    rc = lora.setGain(CDPCFG_RF_LORA_GAIN);
    if (rc == RADIOLIB_ERR_INVALID_GAIN) {
        logerr("ERROR  gain is invalid");
        return DUCKLORA_ERR_SETUP;
    }
    // set the interrupt handler to execute when packet tx or rx is done.
    lora.setDio0Action(config.func,RISING);
#endif

    // set sync word to private network

    rc = lora.setSyncWord(0x12);

    if (rc != RADIOLIB_ERR_NONE) {
        logerr("ERROR  sync word is invalid");
        return DUCKLORA_ERR_SETUP;
    }

    rc = lora.startReceive();

    if (rc != RADIOLIB_ERR_NONE) {
        logerr("ERROR Failed to start receive");
        return DUCKLORA_ERR_RECEIVE;
    }
    return DUCK_ERR_NONE;
}

void DuckRadio::setSyncWord(byte syncWord) {
    int error = lora.setSyncWord(syncWord);
    if (error != RADIOLIB_ERR_NONE) {
        logerr("ERROR  sync word is invalid");
    }
    lora.startReceive();
}

int DuckRadio::readReceivedData(std::vector<byte>* packetBytes) {

    int packet_length = 0;
    int err = DUCK_ERR_NONE;

    packet_length = lora.getPacketLength();

    if (packet_length < MIN_PACKET_LENGTH) {
        logerr("ERROR  handlePacket rx data size invalid: " +
               String(packet_length));

        DuckRadio::setReceiveFlag(false);
        int rxState = startReceive();
        return DUCKLORA_ERR_HANDLE_PACKET;
    }

    loginfo("readReceivedData() - packet length returns: " +
            String(packet_length));

    packetBytes->resize(packet_length);
    err = lora.readData(packetBytes->data(), packet_length);
    loginfo("readReceivedData() - lora.readData returns: " + String(err));

    DuckRadio::setReceiveFlag(false);
    int rxState = startReceive();

    if (err != RADIOLIB_ERR_NONE) {
        logerr("ERROR  readReceivedData failed. err: " + String(err));
        return DUCKLORA_ERR_HANDLE_PACKET;
    }

    loginfo("Rx packet: " + duckutils::convertToHex(packetBytes->data(), packetBytes->size()));
    loginfo("Rx packet: " + duckutils::toString(*packetBytes));

    loginfo("readReceivedData: checking path offset integrity");

    // Do some sanity checks on the received packet here before we continue
    // further RadioLib v4.0.5 has a bug where corrupt packets are still returned
    // to the app despite CRC check being enabled in the radio by both sender and
    // receiver.

    byte* data = packetBytes->data();

    loginfo("readReceivedData: checking data section CRC");

    std::vector<byte> data_section;
    data_section.insert(data_section.end(), &data[DATA_POS], &data[packet_length]);
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
            " size: " + String(packet_length));
#ifndef CDPCFG_RADIO_SX126X
    loginfo("RX: rssi: " + String(lora.getRSSI()) +
            " snr: " + String(lora.getSNR()) +
            " fe: " + String(lora.getFrequencyError(true)) +
            " size: " + String(packet_length));
#else
    loginfo("RX: rssi: " + String(lora.getRSSI()) +
            " snr: " + String(lora.getSNR()) +
            " size: " + String(packet_length));
#endif


    if (rxState != RADIOLIB_ERR_NONE) {
        return rxState;
    }

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

    if (state != RADIOLIB_ERR_NONE) {
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

void DuckRadio::setChannel(int channelNum) {
    logdbg("Setting Channel to : ");
    logdbg(channelNum);

    int err;
    switch(channelNum) {
        case 2:
            loginfo("Set channel: " + String(channelNum));
            err = lora.setFrequency(CHANNEL_2);
            lora.startReceive();
            break;
        case 3:
            loginfo("Set channel: " + String(channelNum));
            err = lora.setFrequency(CHANNEL_3);
            lora.startReceive();
            break;
        case 4:
            loginfo("Set channel: " + String(channelNum));
            err = lora.setFrequency(CHANNEL_4);
            lora.startReceive();
            break;
        case 5:
            loginfo("Set channel: " + String(channelNum));
            err = lora.setFrequency(CHANNEL_5);
            lora.startReceive();
            break;
        case 6:
            loginfo("Set channel: " + String(channelNum));
            err = lora.setFrequency(CHANNEL_6);
            lora.startReceive();
            break;
        case 1:
        default:
            loginfo("Set channel: " + String(channelNum));
            err = lora.setFrequency(CHANNEL_1);
            lora.startReceive();
            break;
    }
    if (err != RADIOLIB_ERR_NONE) {
        logerr("ERROR Failed to set channel");
    } else {
        lora.startReceive();
        channel = channelNum;
        loginfo("Channel Set");
    }
}

void DuckRadio::serviceInterruptFlags() {
    if (DuckRadio::interruptFlags != 0) {
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_TIMEOUT) {
            loginfo("Interrupt flag was set: timeout");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_DONE) {
            loginfo("Interrupt flag was set: packet reception complete");
            DuckRadio::setReceiveFlag(true);
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
            loginfo("Interrupt flag was set: payload CRC error");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_VALID_HEADER) {
            loginfo("Interrupt flag was set: valid header received");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_TX_DONE) {
            loginfo("Interrupt flag was set: payload transmission complete");
            startReceive();
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DONE) {
            loginfo("Interrupt flag was set: CAD complete");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_FHSS_CHANGE_CHANNEL) {
            loginfo("Interrupt flag was set: FHSS change channel");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED) {
            loginfo("Interrupt flag was set: valid LoRa signal detected during CAD operation");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_CMD_CLEAR_IRQ_STATUS) {
            loginfo("Interrupt flag was set: 126X clear IRQ status");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_CMD_CLEAR_DEVICE_ERRORS) {
            loginfo("Interrupt flag was set: 126X clear device errors");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_CRC_ERR ) {
            loginfo("Interrupt flag was set: 126X payload CRC error");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_HEADER_ERR ) {
            loginfo("Interrupt flag was set: 126X header CRC error");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_RX_DONE ) {
            loginfo("Interrupt flag was set: 126X packet reception complete");
            DuckRadio::setReceiveFlag(true);
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_TX_DONE ) {
            loginfo("Interrupt flag was set: 126X payload transmission complete");
            startReceive();
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_TIMEOUT ) {
            loginfo("Interrupt flag was set: 126X timeout");
        }

        DuckRadio::interruptFlags = 0;
    }
}

// IMPORTANT: this function MUST be 'void' type and MUST NOT have any arguments!
void DuckRadio::onInterrupt(void) {
#ifdef CDPCFG_RADIO_SX126X
    interruptFlags = lora.getIrqStatus();
#else
    interruptFlags = lora.getIRQFlags();
#endif
}

int DuckRadio::startTransmitData(byte* data, int length) {
    int err = DUCK_ERR_NONE;
    int tx_err = RADIOLIB_ERR_NONE;
    loginfo("TX data");
    logdbg(" -> " + duckutils::convertToHex(data, length));
    logdbg(" -> length: " + String(length));

    long t1 = millis();
    // this is going to wait for transmission to complete or to timeout
    // when transmit is complete, the Di0 interrupt will be triggered
    tx_err = lora.transmit(data, length);
    switch (tx_err) {
        case RADIOLIB_ERR_NONE:
            loginfo("TX data done in : " + String((millis() - t1)) + "ms");
            // display->log(">> txdone");
            break;

        case RADIOLIB_ERR_PACKET_TOO_LONG:
            // the supplied packet was longer than 256 bytes
            logerr("ERROR startTransmitData too long!");
            err = DUCKLORA_ERR_MSG_TOO_LARGE;
            break;

        case RADIOLIB_ERR_TX_TIMEOUT:
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

    return err;
}
#endif
