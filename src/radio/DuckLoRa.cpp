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

volatile bool DuckLoRa::interruptPending = false;
volatile bool DuckLoRa::receivedFlag = false;

const LoRaConfigParams DuckLoRa::defaultRadioParams = {
    /* band     = */ CDPCFG_RF_LORA_FREQ,
    /* txPower  = */ CDPCFG_RF_LORA_TXPOW,
    /* bw       = */ CDPCFG_RF_LORA_BW,
    /* sf       = */ CDPCFG_RF_LORA_SF,
    /* gain     = */ CDPCFG_RF_LORA_GAIN,
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

int DuckLoRa::checkTxPower(int power) {
    // The range checkLoRaParameters() allows.
    if (power < -9 || power > 22) {
        return DUCK_ERR_INVALID_ARGUMENT;
    }
    // RadioLib's own check, the one setOutputPower() runs before it touches the
    // radio. It only compares numbers, so it works before begin().
#ifdef CDPCFG_RADIO_SX1262
    int16_t rc = lora.checkOutputPower(power, nullptr);
#else
    // SX1278::setOutputPower() uses the RFO output below 2 dBm and PA_BOOST
    // from 2 dBm. Check the output it will use.
    int16_t rc = lora.checkOutputPower(power, nullptr, power < 2);
#endif
    return rc == RADIOLIB_ERR_NONE ? DUCK_ERR_NONE : DUCK_ERR_INVALID_ARGUMENT;
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

    // Ducks only hear radios using the same sync word
    rc = lora.setSyncWord(CDPCFG_RF_LORA_SYNC_WORD);

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
    std::vector<uint8_t> packetBytes;
    int packet_length = 0;
    int err = DUCK_ERR_NONE;
    int rxState = DUCK_ERR_NONE;

    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup %s\n", DUCKLORA_ERR_NOT_INITIALIZED);
        return std::nullopt;
    }

    packet_length = lora.getPacketLength();

    if (packet_length < MIN_PACKET_LENGTH || packet_length > PACKET_LENGTH) {
        logerr_ln("ERROR  handlePacket rx data size invalid: %d", packet_length);
        goToReceiveMode(true); // go back to receive mode and reset the receive flag
        return std::nullopt;
    }

    logdbg_ln("readReceivedData() - packet length returns: %d", packet_length);

    packetBytes.resize(packet_length);
    err = lora.readData(packetBytes.data(), packet_length);
    lastRssi = lora.getRSSI();
    lastSnr = lora.getSNR();
    logdbg_ln("readReceivedData() - lora.readData returns: err = %d", err);

    rxState = goToReceiveMode(true);

    if (err != RADIOLIB_ERR_NONE) {
        logerr_ln("ERROR  readReceivedData failed. err = %d", DUCKLORA_ERR_HANDLE_PACKET);
        return std::nullopt;
    }

    loginfo_ln("Rx packet: %s", duckutils::toString(packetBytes.data(), packetBytes.size()).c_str());

    logdbg_ln("readReceivedData: checking path offset integrity");

    uint8_t* data = packetBytes.data();

    logdbg_ln("readReceivedData: checking data section CRC");

    std::vector<uint8_t> data_section;
    data_section.insert(data_section.end(), &data[DATA_POS], &data[packet_length]);
    uint32_t packet_data_crc = duckutils::toUint32(&data[DATA_CRC_POS]);
    uint32_t computed_data_crc =
            CRC32::calculate(data_section.data(), data_section.size());
    if (computed_data_crc != packet_data_crc) {
        lastReceiveTime = millis(); //even if the packet is invalid, we need to know when we last received
        logerr_ln("ERROR data crc mismatch: received: 0x%X, calculated: 0x%X",packet_data_crc, computed_data_crc);
        return std::nullopt;
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
    delay(length);
    return startTransmitData(data, length);
}

void DuckLoRa::delay(size_t size) {
    // Delay the transmission if we have received within the last 5 seconds
    if ((millis() - this->lastReceiveTime) < 5000L) {
        std::uniform_int_distribution<> distrib(0, 3000L);
        std::chrono::milliseconds txdelay(distrib(gen));
        
        // Add the time on air to the delay
        // txdelay_ms += lora.getTimeOnAir(size);

        logdbg_ln("Last receive was %ld ms ago, delaying transmission by %ld ms", millis() - this->lastReceiveTime, txdelay.count());

        // Use FreeRTOS task delay, which will not block other tasks
        // must pass ticks, so convert from ms to ticks
        vTaskDelay(txdelay.count());
    }
}

int DuckLoRa::sendData(std::vector<uint8_t> data)
{
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }
    delay(data.size());
    return startTransmitData(data.data(), data.size());
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
    return lastRssi;
}

float DuckLoRa::getSNR()
{
    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }
    return lastSnr;
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
    // A missed IRQ must not leave the software permanently busy.
    if (!interruptPending && transmitting && uint32_t(millis() - txStartedAt) >= txTimeoutMs) {
        transmitting = false;
        ++txTimeoutCount;
        lora.finishTransmit();
        goToReceiveMode(false);
    }
    if (!DuckLoRa::interruptPending) {
        return;
    }

    // Clear the pending latch BEFORE reading the flags off the radio. If DIO1
    // fires again while we are servicing, the ISR re-sets interruptPending and
    // we pick the new event up on the next loop instead of silently dropping it
    // (the read-then-clear race that could leave the node parked in standby).
    DuckLoRa::interruptPending = false;

    // Safe to touch SPI here: this runs in loop context, not the ISR.
    // getIrqFlags() returns uint32_t -- do not narrow it to uint16_t.
    uint32_t flags = lora.getIrqFlags();
    if (flags == 0) {
        // The latch fired but the radio reports nothing. This happens when another
        // path (e.g. goToReceiveMode() from readReceivedData()) cleared the IRQ
        // status between the ISR and this call; that path has already re-armed the
        // radio, so there is nothing to do and nothing to recover.
        return;
    }

#ifdef CDPCFG_RADIO_SX1262
        // SX1262 flags.
        //
        // Do NOT test these against RADIOLIB_SX126X_CMD_* constants: those are SPI
        // command opcodes, not IRQ bits, and they alias real ones
        // (CMD_CLEAR_IRQ_STATUS == 0x02 == IRQ_RX_DONE,
        //  CMD_CLEAR_DEVICE_ERRORS == 0x07 == TX_DONE|RX_DONE|PREAMBLE_DETECTED),
        // so every received packet used to emit two meaningless log lines.
        const bool rxError =
            (flags & (RADIOLIB_SX126X_IRQ_CRC_ERR | RADIOLIB_SX126X_IRQ_HEADER_ERR)) != 0;

        if (rxError) {
            // A corrupt frame raises RX_DONE *together with* CRC_ERR/HEADER_ERR, so
            // this must be exclusive with the RX_DONE branch below. Previously both
            // ran: goToReceiveMode() cleared the IRQ status and then the RX_DONE
            // branch set the receive flag anyway, so SX126x::readData() -- which
            // re-reads the IRQ register to decide RADIOLIB_ERR_CRC_MISMATCH -- saw
            // irq == 0 and handed a known-corrupt frame up as valid.
            if (flags & RADIOLIB_SX126X_IRQ_CRC_ERR) {
                logdbg_ln("SX1262 Interrupt flag was set: payload CRC error");
            }
            if (flags & RADIOLIB_SX126X_IRQ_HEADER_ERR) {
                logdbg_ln("SX1262 Interrupt flag was set: header CRC error");
            }
            // goToReceiveMode() re-arms the radio (startReceive clears the IRQ
            // status). Do NOT call lora.standby() afterwards: that immediately
            // knocks the radio back out of receive and the node goes deaf.
            goToReceiveMode(false);
        } else if (flags & RADIOLIB_SX126X_IRQ_RX_DONE) {
            logdbg_ln("SX1262 Interrupt flag was set: packet reception complete");
            setReceiveFlag(true);
            // We are done receiving, go to standby. We can't sleep because the read
            // buffer is not empty. The IRQ status is deliberately NOT cleared here:
            // SX126x::readData() re-reads it to detect CRC/header errors and clears
            // it itself once the payload has been read out.
            lora.standby();
        }

        if (flags & RADIOLIB_SX126X_IRQ_TX_DONE ) {
            transmitting = false;
            ++txDoneCount;
            loginfo_ln("[CDP_DIAG] TX event=done count=%lu", (unsigned long)txDoneCount);
            logdbg_ln("SX1262 Interrupt flag was set: payload transmission complete");
            lora.finishTransmit();
            goToReceiveMode(false);
        }
        if (flags & RADIOLIB_SX126X_IRQ_TIMEOUT ) {
            if (transmitting) ++txTimeoutCount;
            transmitting = false;
            logdbg_ln("SX1262 Interrupt flag was set: timeout");
            goToReceiveMode(false);
        }
#else
        // SX127X flags
        if (flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_TIMEOUT) {
            goToReceiveMode(true); // go back to receive mode and reset the receive flag
            logdbg_ln("SX127x Interrupt flag was set: timeout");
        }
        // Exclusive for the same reason as the SX1262 path above: a corrupt frame
        // raises RX_DONE and PAYLOAD_CRC_ERROR together, and accepting it as a
        // completed reception hands a known-bad packet to the caller.
        if (flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_PAYLOAD_CRC_ERROR) {
            goToReceiveMode(true); // go back to receive mode and reset the receive flag
            logdbg_ln("SX127x Interrupt flag was set: payload CRC error");
        } else if (flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_RX_DONE) {
            logdbg_ln("SX127x Interrupt flag was set: packet reception complete");
            setReceiveFlag(true); // set the receive flag and we stay in receive mode
            lora.standby(); // we are done receiving, go to standby. We can't sleep because read buffer is not empty
        }
        if (flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_VALID_HEADER) {
            logdbg_ln("SX127x Interrupt flag was set: valid header received");
        }
        if (flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_TX_DONE) {
            transmitting = false;
            ++txDoneCount;
            loginfo_ln("[CDP_DIAG] TX event=done count=%lu", (unsigned long)txDoneCount);
            logdbg_ln("SX127x Interrupt flag was set: payload transmission complete");
            goToReceiveMode(false); // go back to receive mode and reset the receive flag
        }
        if (flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DONE) {
            logdbg_ln("SX127x Interrupt flag was set: CAD complete");
        }
        if (flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_FHSS_CHANGE_CHANNEL) {
            logdbg_ln("SX127x Interrupt flag was set: FHSS change channel");
        }
        if (flags & RADIOLIB_SX127X_CLEAR_IRQ_FLAG_CAD_DETECTED) {
            logdbg_ln("SX127x Interrupt flag was set: valid LoRa signal detected during CAD operation");
        }
#endif
}

// IMPORTANT: this function MUST be 'void' type and MUST NOT have any arguments!
void DuckLoRa::onInterrupt(void) {
    // ISR context: do NOT touch the SPI bus here. lora.getIrqFlags() issues a
    // full SPI transaction (including a BUSY-line wait and the arduino-esp32 SPI
    // HAL lock) which can deadlock or corrupt the bus if DIO1 fires while the
    // main loop is mid-transfer (startTransmit / readData / startReceive). The
    // documented RadioLib pattern is for the ISR to only latch a flag; the flags
    // are then read from loop context in serviceInterruptFlags().
    interruptPending = true;
}

int DuckLoRa::startTransmitData(uint8_t* data, int length) {
    serviceInterruptFlags();
    if (transmitting || getReceiveFlag()) return DUCKLORA_ERR_TX_BUSY;
    int err = DUCK_ERR_NONE;
    int tx_err = RADIOLIB_ERR_NONE;

    if (!isSetup) {
        logerr_ln("ERROR  LoRa radio not setup");
        return DUCKLORA_ERR_NOT_INITIALIZED;
    }

    loginfo_ln("TX data");
    logdbg_ln(" -> len: %d, %s", length, duckutils::toString(data, length).c_str());

    long t1 = millis();
    // non blocking transmit
    tx_err = lora.startTransmit(data, length);
    switch (tx_err) {
        case RADIOLIB_ERR_NONE:
            transmitting = true;
            txStartedAt = millis();
            txTimeoutMs = lora.getTimeOnAir(length) / 1000UL + 2000UL;
            logdbg_ln("TX data done in : %d ms",(millis() - t1));
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
