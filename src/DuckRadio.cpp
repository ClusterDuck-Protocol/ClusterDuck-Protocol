#include "include/DuckRadio.h"
#ifdef CDPCFG_RADIO_SX1262
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

#include "include/DuckUtils.h"
#include <RadioLib.h>

#if defined(CDPCFG_RADIO_SX1262)
CDPCFG_LORA_CLASS lora =
        new Module(CDPCFG_PIN_LORA_CS, CDPCFG_PIN_LORA_DIO1, CDPCFG_PIN_LORA_RST,
                   CDPCFG_PIN_LORA_BUSY);
#else
CDPCFG_LORA_CLASS lora = new Module(CDPCFG_PIN_LORA_CS, CDPCFG_PIN_LORA_DIO0,
                  CDPCFG_PIN_LORA_RST, CDPCFG_PIN_LORA_DIO1);
#endif

volatile uint16_t DuckRadio::interruptFlags = 0;
volatile bool DuckRadio::receivedFlag = false;

DuckRadio::DuckRadio() {}

int DuckRadio::setupRadio(LoraConfigParams config) {
    loginfo_ln("Setting up RADIOLIB LoRa radio...");
    int rc;
    rc = lora.begin();
    if (rc != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  initializing LoRa driver. state = %d", rc);
        return DUCKLORA_ERR_BEGIN;
    }
    // Lora is started, we need to set all the radio parameters, before it can
    // start receiving packets

    loginfo_ln("Setting up LoRa radio parameters...");
    rc = lora.setFrequency(config.band);
    if (rc == RADIOLIB_ERR_INVALID_FREQUENCY) {
        logerr_ln("ERROR  frequency is invalid");
        return DUCKLORA_ERR_SETUP;
    }

    rc = lora.setBandwidth(config.bw);
    if (rc == RADIOLIB_ERR_INVALID_BANDWIDTH) {
        logerr_ln("ERROR  bandwidth is invalid");
        return DUCKLORA_ERR_SETUP;
    }

    rc = lora.setSpreadingFactor(config.sf);
    if (rc == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
        logerr_ln("ERROR  spreading factor is invalid");
        return DUCKLORA_ERR_SETUP;
    }

    rc = lora.setOutputPower(config.txPower);
    if (rc == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
        logerr_ln("ERROR  output power is invalid");
        return DUCKLORA_ERR_SETUP;
    }
#ifdef CDPCFG_RADIO_SX1262
    // set the interrupt handler to execute when packet tx or rx is done.
    lora.setDio1Action(config.func);
#else
    rc = lora.setGain(CDPCFG_RF_LORA_GAIN);
    if (rc == RADIOLIB_ERR_INVALID_GAIN) {
        logerr_ln("ERROR  gain is invalid");
        return DUCKLORA_ERR_SETUP;
    }
    // set the interrupt handler to execute when packet tx or rx is done.
    lora.setDio0Action(config.func,RISING);
#endif

    // set sync word to private network

    rc = lora.setSyncWord(0x12);

    if (rc != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  sync word is invalid");
        return DUCKLORA_ERR_SETUP;
    }
/*
    #ifdef HELTEC_CUBE_CELL
    rc = lora.setTCXO(1.8, 2000);
    if (rc == RADIOLIB_ERR_INVALID_TCXO_VOLTAGE) {
        logerr_ln("ERROR  TCXO failed!");
        return DUCKLORA_ERR_SETUP;
    }

    rc = lora.setDio2AsRfSwitch();
    if (rc != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  setDio2AsRfSwitch failed!");
        return DUCKLORA_ERR_SETUP;
    }
    #endif
 */   

    rc = lora.startReceive();

    if (rc != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR Failed to start receive");
        return DUCKLORA_ERR_RECEIVE;
    }
    loginfo_ln("LoRa radio setup complete");
    return DUCK_ERR_NONE;
}

void DuckRadio::setSyncWord(byte syncWord) {
    int error = lora.setSyncWord(syncWord);
    if (error != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  sync word is invalid");
    }
    lora.startReceive();
}

int DuckRadio::goToReceiveMode(bool clearReceiveFlag) {
    if (clearReceiveFlag) {
      DuckRadio::setReceiveFlag(false);
    }
    return startReceive();
}

int DuckRadio::readReceivedData(std::vector<byte>* packetBytes) {

    int packet_length = 0;
    int err = DUCK_ERR_NONE;
    int rxState = DUCK_ERR_NONE;
    packet_length = lora.getPacketLength();

    if (packet_length < MIN_PACKET_LENGTH) {
        logerr_ln("ERROR  handlePacket rx data size invalid: %d", packet_length);

        rxState = goToReceiveMode(true);
        if (rxState != DUCK_ERR_NONE) {
            return rxState;
        }
        return DUCKLORA_ERR_HANDLE_PACKET;
    }

    loginfo_ln("readReceivedData() - packet length returns: %d", packet_length);

    packetBytes->resize(packet_length);
    err = lora.readData(packetBytes->data(), packet_length);
    loginfo_ln("readReceivedData() - lora.readData returns: err = %d", err);

    rxState = goToReceiveMode(true);

    if (err != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  readReceivedData failed. err = %d", err);
        return DUCKLORA_ERR_HANDLE_PACKET;
    }

    loginfo_ln("Rx packet: %s", duckutils::convertToHex(packetBytes->data(), packetBytes->size()).c_str());
    loginfo_ln("Rx packet: %s", duckutils::toString(*packetBytes).c_str());

    loginfo_ln("readReceivedData: checking path offset integrity");

    byte* data = packetBytes->data();

    loginfo_ln("readReceivedData: checking data section CRC");

    std::vector<byte> data_section;
    data_section.insert(data_section.end(), &data[DATA_POS], &data[packet_length]);
    uint32_t packet_data_crc = duckutils::toUint32(&data[DATA_CRC_POS]);
    uint32_t computed_data_crc =
            CRC32::calculate(data_section.data(), data_section.size());
    if (computed_data_crc != packet_data_crc) {
        logerr_ln("ERROR data crc mismatch: received: 0x%X, calculated: 0x%X",packet_data_crc, computed_data_crc);
        return DUCKLORA_ERR_HANDLE_PACKET;
    }
    // we have a good packet
#ifndef CDPCFG_RADIO_SX1262
    loginfo_ln("RX: rssi: %f snr: %f fe: %d size: %d", lora.getRSSI(), lora.getSNR(), lora.getFrequencyError(true), packet_length);
#else
    loginfo_ln("RX: rssi: %f snr: %f size: %d", lora.getRSSI(), lora.getSNR(), packet_length);
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
        logerr_ln("ERROR startReceive failed, code %d", state);
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

    logdbg_ln("Setting Channel to : %d", channelNum);

    int err;
    switch(channelNum) {
        case 2:
            err = lora.setFrequency(CHANNEL_2);
            lora.startReceive();
            break;
        case 3:
            err = lora.setFrequency(CHANNEL_3);
            lora.startReceive();
            break;
        case 4:
            err = lora.setFrequency(CHANNEL_4);
            lora.startReceive();
            break;
        case 5:
            err = lora.setFrequency(CHANNEL_5);
            lora.startReceive();
            break;
        case 6:
            err = lora.setFrequency(CHANNEL_6);
            lora.startReceive();
            break;
        case 1:
        default:
            err = lora.setFrequency(CHANNEL_1);
            lora.startReceive();
            break;
    }
    if (err != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR Failed to set channel");
    } else {
        lora.startReceive();
        channel = channelNum;
        loginfo_ln("Channel Set");
    }
}

void DuckRadio::serviceInterruptFlags() {
    if (DuckRadio::interruptFlags != 0) {

        // SX127X flags
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_TIMEOUT) {
            loginfo_ln("SX127x Interrupt flag was set: timeout");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_DONE) {
            loginfo_ln("SX127x Interrupt flag was set: packet reception complete");
            DuckRadio::goToReceiveMode(true);
            lora.standby(); // we are done receiving, go to standby. We can't sleep because read buffer is not empty
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
            loginfo_ln("SX127x Interrupt flag was set: payload CRC error");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_VALID_HEADER) {
            loginfo_ln("SX127x Interrupt flag was set: valid header received");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_TX_DONE) {
            loginfo_ln("SX127x Interrupt flag was set: payload transmission complete");
            DuckRadio::goToReceiveMode(false);
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DONE) {
            loginfo_ln("SX127x Interrupt flag was set: CAD complete");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_FHSS_CHANGE_CHANNEL) {
            loginfo_ln("SX127x Interrupt flag was set: FHSS change channel");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED) {
            loginfo_ln("SX127x Interrupt flag was set: valid LoRa signal detected during CAD operation");
        }

        // SX1262 flags
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_CMD_CLEAR_IRQ_STATUS) {
            loginfo_ln("SX1262 Interrupt flag was set: clear IRQ status");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_CMD_CLEAR_DEVICE_ERRORS) {
            loginfo_ln("SX1262 Interrupt flag was set: clear device errors");
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_CRC_ERR ) {
            loginfo_ln("SX1262 Interrupt flag was set: payload CRC error");
            DuckRadio::goToReceiveMode(false);
            lora.standby();
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_HEADER_ERR ) {
            loginfo_ln("SX1262 Interrupt flag was set: header CRC error");
            DuckRadio::goToReceiveMode(false);
            lora.standby();
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_RX_DONE ) {
            loginfo_ln("SX1262 Interrupt flag was set: packet reception complete");
            DuckRadio::setReceiveFlag(true);
            lora.standby(); // we are done receiving, go to standby. We can't sleep because read buffer is not empty
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_TX_DONE ) {
            loginfo_ln("SX1262 Interrupt flag was set: payload transmission complete");
            lora.finishTransmit();
            DuckRadio::goToReceiveMode(false);
        }
        if (DuckRadio::interruptFlags & RADIOLIB_SX126X_IRQ_TIMEOUT ) {
            loginfo_ln("SX1262 Interrupt flag was set: timeout");
            DuckRadio::goToReceiveMode(false);
        }

        DuckRadio::interruptFlags = 0;
    }
}

// IMPORTANT: this function MUST be 'void' type and MUST NOT have any arguments!
void DuckRadio::onInterrupt(void) {
#ifdef CDPCFG_RADIO_SX1262
    interruptFlags = lora.getIrqStatus();
#else
    interruptFlags = lora.getIRQFlags();
#endif
}

int DuckRadio::startTransmitData(byte* data, int length) {
    int err = DUCK_ERR_NONE;
    int tx_err = RADIOLIB_ERR_NONE;
    loginfo_ln("TX data");
    logdbg_ln(" -> len: %d, %s", length, duckutils::convertToHex(data, length).c_str());

    long t1 = millis();
    // this is going to wait for transmission to complete or to timeout
    // when transmit is complete, the Di0 interrupt will be triggered
    tx_err = lora.startTransmit(data, length);
    switch (tx_err) {
        case RADIOLIB_ERR_NONE:
            loginfo_ln("TX data done in : %d ms",(millis() - t1));
            break;

        case RADIOLIB_ERR_PACKET_TOO_LONG:
            // the supplied packet was longer than 256 bytes
            logerr_ln("ERROR startTransmitData too long!");
            err = DUCKLORA_ERR_MSG_TOO_LARGE;
            break;

        case RADIOLIB_ERR_TX_TIMEOUT:
            logerr_ln("ERROR startTransmitData timeout!");
            display->log("tx-tmout");
            err = DUCKLORA_ERR_TIMEOUT;
            break;

        default:
            logerr_ln("ERROR startTransmitData failed, err: %d", tx_err);
            display->log("tx-fail");
            err = DUCKLORA_ERR_TRANSMIT;
            break;
    }

    return err;
}
