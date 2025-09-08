#include "DuckLoRa.h"
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

#if defined(CDPCFG_RADIO_SX1262)
CDPCFG_LORA_CLASS lora =
        new Module(CDPCFG_PIN_LORA_CS, CDPCFG_PIN_LORA_DIO1, CDPCFG_PIN_LORA_RST,
                   CDPCFG_PIN_LORA_BUSY);
#else
CDPCFG_LORA_CLASS lora = new Module(CDPCFG_PIN_LORA_CS, CDPCFG_PIN_LORA_DIO0,
                  CDPCFG_PIN_LORA_RST, CDPCFG_PIN_LORA_DIO1);
#endif

#define RSSI_MAX (-20.0f)
#define RSSI_MIN (-131.0f)
#define SNR_MAX 11.5f
#define SNR_MIN (-11.5f)

volatile uint16_t DuckLoRa::interruptFlags = 0;
volatile bool DuckLoRa::receivedFlag = false;

const LoRaConfigParams DuckLoRa::defaultRadioParams = {
    /* band     = */ 915.0f,
    /* txPower  = */ 14,
    /* bw       = */ 125.0f,
    /* sf       = */ 7,
    /* gain     = */ 0,
    /* func     = */ onInterrupt
};

int DuckLoRa::checkLoRaParameters(LoRaConfigParams config) { //this can be improved
    int rc = DUCK_ERR_NONE;
    if (config.func == NULL) {
        logerr_ln("ERROR  interrupt function is NULL");
        return DUCK_ERR_INVALID_ARGUMENT;
    }
    if (config.sf < 6 || config.sf > 12) {
        logerr_ln("ERROR  spreading factor is invalid");
        return DUCK_ERR_INVALID_ARGUMENT;
    }
    if (config.band < 150.0 || config.band > 960.0) {
        logerr_ln("ERROR  frequency is invalid");
        return DUCK_ERR_INVALID_ARGUMENT;
    }
    if (config.txPower < -9 || config.txPower > 22) {
        logerr_ln("ERROR  tx power is invalid");
        return DUCK_ERR_INVALID_ARGUMENT;
    }
    if (config.bw < 7.8 || config.bw > 500.0) {
        logerr_ln("ERROR  bandwidth is invalid");
        return DUCK_ERR_INVALID_ARGUMENT;
    }
    if (config.gain < 0 || config.gain > 3) {
        logerr_ln("ERROR  gain is invalid");
        return DUCK_ERR_INVALID_ARGUMENT;
    }
    return rc;
}

int DuckLoRa::setupRadio(const LoRaConfigParams &config) {
    loginfo_ln("Setting up RADIOLIB LoRa radio...");
    int rc;
    rc = checkLoRaParameters(config);
    if (rc != DUCK_ERR_NONE) {
        return rc;
    }

    if (isSetup) {
        loginfo_ln("LoRa radio already setup");
        return DUCK_ERR_NONE;
    }

    rc = lora.begin();
    if (rc != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  initializing LoRa driver. state = %d", rc);
        return DUCKLORA_ERR_BEGIN;
    }
    
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

    rc = lora.setSyncWord(0x12); //should this be passed?

    if (rc != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  sync word is invalid");
        return DUCKLORA_ERR_SETUP;
    }   

    rc = lora.startReceive();

    if (rc != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR Failed to start receive");
        return DUCKLORA_ERR_RECEIVE;
    }

    loginfo_ln("LoRa radio setup complete");
    isSetup = true;
    return DUCK_ERR_NONE;
}

int DuckLoRa::goToReceiveMode(bool clearReceiveFlag) {
    if (clearReceiveFlag) {
      DuckLoRa::setReceiveFlag(false);
    }
    return startReceive();
}

std::optional<std::vector<uint8_t>> DuckLoRa::readReceivedData() { //return a std optional
    std::vector<uint8_t>* packetBytes;
    int packet_length = 0;
    int err = DUCK_ERR_NONE;
    int rxState = DUCK_ERR_NONE;

    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup %s\n", DUCKLORA_ERR_NOT_INITIALIZED);
        return std::nullopt;
    }

    packet_length = lora.getPacketLength();

    if (packet_length < MIN_PACKET_LENGTH) {
        logerr_ln("ERROR  handlePacket rx data size invalid: %d", packet_length);

        rxState = goToReceiveMode(true); // go back to receive mode and reset the receive flag
    }

    loginfo_ln("readReceivedData() - packet length returns: %d", packet_length);

    packetBytes->resize(packet_length);
    err = lora.readData(packetBytes->data(), packet_length);
    loginfo_ln("readReceivedData() - lora.readData returns: err = %d", err);

    rxState = goToReceiveMode(true);

    if (err != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  readReceivedData failed. err = %d", DUCKLORA_ERR_HANDLE_PACKET);
    }

    loginfo_ln("Rx packet: %s", duckutils::convertToHex(packetBytes->data(), packetBytes->size()).c_str());

    loginfo_ln("readReceivedData: checking path offset integrity");

    byte* data = packetBytes->data();

    loginfo_ln("readReceivedData: checking data section CRC");

    std::vector<uint8_t> data_section;
    data_section.insert(data_section.end(), &data[DATA_POS], &data[packet_length]);
    uint32_t packet_data_crc = duckutils::toUint32(&data[DATA_CRC_POS]);
    uint32_t computed_data_crc =
            CRC32::calculate(data_section.data(), data_section.size());
    if (computed_data_crc != packet_data_crc) {
        lastReceiveTime = millis(); //even if the packet is invalid, we need to know when we last received
        logerr_ln("ERROR data crc mismatch: received: 0x%X, calculated: 0x%X",packet_data_crc, computed_data_crc);
    }
    
    #ifndef CDPCFG_RADIO_SX1262
        loginfo_ln("RX: rssi: %f snr: %f fe: %d size: %d", lora.getRSSI(), lora.getSNR(), lora.getFrequencyError(true), packet_length);
    #else
        loginfo_ln("RX: rssi: %f snr: %f size: %d", lora.getRSSI(), lora.getSNR(), packet_length);
    #endif


    if (rxState != RADIOLIB_ERR_NONE) {
        lastReceiveTime = millis(); //even if rxState is bad, we need to know when we last received
        return std::nullopt;
    }
    lastReceiveTime = millis(); // always update the last receive time
    std::vector<uint8_t> packetVector(data, data + packet_length);
    return packetVector;
}

int DuckLoRa::sendData(uint8_t* data, int length)
{

    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }
    return startTransmitData(data, length);
}

void DuckLoRa::delay() {
    //Delay the transmission if we have received within the last 5 seconds
    if((millis() - this->lastReceiveTime) < 5000L) {
        std::uniform_int_distribution<> distrib(0, 3000L);
        std::chrono::milliseconds txdelay(distrib(gen));
        //Random delay between 0 and 3 seconds
        std::chrono::duration<long, std::milli> txdelay_ms(txdelay.count());

        unsigned long current_time = millis();
        unsigned long previousMillis = 0;
        while(previousMillis - current_time <= txdelay_ms.count()) {
            previousMillis = millis();
            loginfo_ln("Delaying transmission for %ld ms", txdelay_ms.count());
        }
    }
}

int DuckLoRa::sendData(std::vector<uint8_t> data)
{
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }
    delay();
    return startTransmitData(data.data(), data.size());
}

//internal convenience method for now, may use in the routing branch later
void DuckLoRa::getSignalScore()
{

    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return;
    }
    //Normalize the values to 0-10 range
    signalInfo.rssi = (getRSSI() - RSSI_MIN)/(RSSI_MAX-RSSI_MIN);
    signalInfo.snr = (getSNR() - SNR_MIN)/(SNR_MAX-SNR_MIN);
    signalInfo.signalScore = (signalInfo.rssi + signalInfo.snr) / 2.0f;
}

int DuckLoRa::startReceive()
{
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }
    int state = lora.startReceive();

    if (state != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR startReceive failed, code %d", state);
        return DUCKLORA_ERR_RECEIVE;
    }

    return DUCK_ERR_NONE;
}

float DuckLoRa::getRSSI()
{ 
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }
    return lora.getRSSI(); 
}

float DuckLoRa::getSNR()
{
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }
    return lora.getSNR();
}

int DuckLoRa::standBy()
{ 
    int rc = DUCK_ERR_NONE;
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }
    if (lora.standby() != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  standby failed");
        rc = DUCKLORA_ERR_STANDBY;
    }
    return rc;
}

int DuckLoRa::sleep()
{ 
    int rc = DUCK_ERR_NONE;

    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }    
    if (lora.sleep() != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  sleep failed");
        rc = DUCKLORA_ERR_SLEEP;
    }
    return rc;
}

void DuckLoRa::serviceInterruptFlags() {
    if (DuckLoRa::interruptFlags != 0) {

#ifdef CDPCFG_RADIO_SX1262
        // SX1262 flags
        if (DuckLoRa::interruptFlags & RADIOLIB_SX126X_CMD_CLEAR_IRQ_STATUS) {
            loginfo_ln("SX1262 Interrupt flag was set: clear IRQ status");
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX126X_CMD_CLEAR_DEVICE_ERRORS) {
            loginfo_ln("SX1262 Interrupt flag was set: clear device errors");
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX126X_IRQ_CRC_ERR ) {
            loginfo_ln("SX1262 Interrupt flag was set: payload CRC error");
            goToReceiveMode(false);
            lora.standby();
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX126X_IRQ_HEADER_ERR ) {
            loginfo_ln("SX1262 Interrupt flag was set: header CRC error");
            goToReceiveMode(false);
            lora.standby();
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX126X_IRQ_RX_DONE ) {
            loginfo_ln("SX1262 Interrupt flag was set: packet reception complete");
            setReceiveFlag(true);
            lora.standby(); // we are done receiving, go to standby. We can't sleep because read buffer is not empty
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX126X_IRQ_TX_DONE ) {
            loginfo_ln("SX1262 Interrupt flag was set: payload transmission complete");
            lora.finishTransmit();
            goToReceiveMode(false);
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX126X_IRQ_TIMEOUT ) {
            loginfo_ln("SX1262 Interrupt flag was set: timeout");
            goToReceiveMode(false);
        }
#else
        // SX127X flags
        if (DuckLoRa::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_TIMEOUT) {
            goToReceiveMode(true); // go back to receive mode and reset the receive flag
            loginfo_ln("SX127x Interrupt flag was set: timeout");
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_DONE) {
            loginfo_ln("SX127x Interrupt flag was set: packet reception complete");
            setReceiveFlag(true); // set the receive flag and we stay in receive mode
            lora.standby(); // we are done receiving, go to standby. We can't sleep because read buffer is not empty
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
            goToReceiveMode(true); // go back to receive mode and reset the receive flag
            loginfo_ln("SX127x Interrupt flag was set: payload CRC error");
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_VALID_HEADER) {
            loginfo_ln("SX127x Interrupt flag was set: valid header received");
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_TX_DONE) {
            loginfo_ln("SX127x Interrupt flag was set: payload transmission complete");
            goToReceiveMode(false); // go back to receive mode and reset the receive flag
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DONE) {
            loginfo_ln("SX127x Interrupt flag was set: CAD complete");
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_FHSS_CHANGE_CHANNEL) {
            loginfo_ln("SX127x Interrupt flag was set: FHSS change channel");
        }
        if (DuckLoRa::interruptFlags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED) {
            loginfo_ln("SX127x Interrupt flag was set: valid LoRa signal detected during CAD operation");
        }
#endif
        DuckLoRa::interruptFlags = 0;
    }
}

// IMPORTANT: this function MUST be 'void' type and MUST NOT have any arguments!
void DuckLoRa::onInterrupt(void) {
    interruptFlags = lora.getIrqFlags();
}

int DuckLoRa::startTransmitData(uint8_t* data, int length) {
    int err = DUCK_ERR_NONE;
    int tx_err = RADIOLIB_ERR_NONE;

    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }

    loginfo_ln("TX data");
    logdbg_ln(" -> len: %d, %s", length, duckutils::convertToHex(data, length).c_str());

    long t1 = millis();
    // non blocking transmit
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
            err = DUCKLORA_ERR_TIMEOUT;
            break;

        default:
            logerr_ln("ERROR startTransmitData failed, err: %d", tx_err);
            err = DUCKLORA_ERR_TRANSMIT;
            break;
    }

    return err;
}
