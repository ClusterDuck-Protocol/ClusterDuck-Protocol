/**
 * @file DuckLoRa.h
 * @brief This file is internal to CDP and provides the library access to
 * onboard LoRa module functions.
 * @version
 * @date 2025-7-15
 *
 * @copyright
 */

#ifndef DUCKLORA_H_
#define DUCKLORA_H_
 
#include "CdpPacket.h"
#include "utils/DuckError.h"
#include "utils/DuckLogger.h"
#include "utils/DuckUtils.h"
#include <RadioLib.h>
#include <memory>
#include <chrono>
#include <random>//replace to reduce program size

/**
 * @brief Internal structure to hold the LoRa module configuration
 * 
 */
struct LoRaConfigParams {
    /// radio frequency (i.e US915Mhz)
    float band;
    /// SPI slave select pin - the pin on each device that the master can use to enable and disable specific devices.
    int8_t txPower;
    /// bandwidth
    float bw;
    /// spreading factor
    uint8_t sf;
    /// gain
    uint8_t gain;
    /// interrupt service routine function when di0 activates
    void (*func)(void); 
};

// const struct LoRaPins {
//     const int ss;
//     /// chip reset pin
//     const int rst;
//     /// dio0 interrupt pin
//     const int di0;
//     /// dio1 interrupt pin
//     const int di1;
//     /// transmit power
// }
  

class DuckLoRa {

    public:
        DuckLoRa() {
            // Use hardware RNG on ESP32 for proper entropy; fall back to time() on Linux/PC
            #ifdef ARDUINO
                gen.seed(esp_random());
            #else
                gen.seed(time(nullptr));
            #endif
        };
        DuckLoRa(DuckLoRa const&) = delete;
        DuckLoRa& operator=(DuckLoRa const&) = delete;

        static const LoRaConfigParams defaultRadioParams;

                /**
         * @brief Initialize the LoRa chip.
         * 
         * @param config    lora configurstion parameters
         * @returns 0 if initialization was successful, an error code otherwise. 
         */
        int setupRadio(const LoRaConfigParams& config = defaultRadioParams);

                /**
         * @brief Send packet data out into the LoRa mesh network
         *
         * @param data byte buffer to send
         * @param length length of the byte buffer
         * @return int
         */
        int sendData(uint8_t* data, int length);

        /**
         * @brief Send packet data out into the mesh network
         *
         * @param data byte vector to send
         * @returns DUCK_ERR_NONE if the message was sent successfully, an error code otherwise.
         */
        int sendData(std::vector<uint8_t> data);

        /**
         * @brief Get the data received from the radio
         * 
         * @return DUCK_ERR_NONE if the chip is sucessfuly set in standby mode, an error code otherwise. 
         */
        std::optional<std::vector<uint8_t>> readReceivedData(); //can this be CdpPacket optional instead?

        /**
         * @brief Service the RadioLib SX127x and SX126x interrupt flags.
         * 
         */
        void serviceInterruptFlags();

        /**
         * @brief Get the data receive flag.
         * 
         * @return true if the flag is set, false otherwise.
         */
        static bool getReceiveFlag() { return receivedFlag; }

        /**
         * @brief Get the current RSSI value.
         *
         * @returns A float representing the rssi value.
         */

         float getRSSI();

        /**
         * @brief Get the current SNR value.
         *
         * @returns A float representing the snr value.
         */
        float getSNR();

        /**
         * @brief Temporarily switch the radio to a randomly selected uplink
         *        channel from CDPCFG_UPLINK_CHANNEL_POOL.
         *
         * Called by Duck::sendToRadio() immediately before transmitting a
         * Papa-bound packet that *originated* from this duck
         * (sduid == this->duid). Relayed packets are never subject to channel
         * switching and are always sent on the shared mesh channel
         * (CDPCFG_RF_LORA_FREQ, 922.8 MHz) so that other MamaDucks can
         * continue to hear and forward them.
         *
         * The mesh channel is automatically restored in the TX_DONE interrupt
         * handler after the transmission completes.
         *
         * @param freq Uplink frequency in MHz (must be a member of
         *             CDPCFG_UPLINK_CHANNEL_POOL)
         * @returns DUCK_ERR_NONE on success, an error code otherwise.
         */
        int setUplinkFrequency(float freq);

        /**
         * @brief Pick a random channel from CDPCFG_UPLINK_CHANNEL_POOL using
         *        the internal Mersenne-Twister RNG.
         *
         * Each of the 8 AS923 channels (921.4–922.8 MHz, 200 kHz steps) is
         * equally likely to be selected, spreading uplink traffic across all
         * SX1302 demodulators.
         *
         * @returns A frequency in MHz.
         */
        float getRandomUplinkChannel();

    private:
        static volatile uint16_t interruptFlags;
        static volatile bool receivedFlag;
        volatile bool isSetup = false;
        unsigned long lastReceiveTime = 0L;

        static void setReceiveFlag(bool value) { receivedFlag = value; }

        int goToReceiveMode(bool clear);
        int checkLoRaParameters(LoRaConfigParams config);
        /**
         * @brief Introduce a random delay based on the size of the data to be sent.
         * This is to help reduce collisions on the network.
         *
         * @param size size of the data to be sent. Will be used to get time on air.
         */
        void delay(size_t size);
        std::mt19937 gen;

        /**
         * @brief Set the Duck to be ready to recieve LoRa packets.
         *
         * @returns DUCK_ERR_NONE if the call was successful, an error code otherwise.
         */
        int startReceive();

        /**
         * @brief Set the Duck to be ready to transmit packets.
         *
         * @param data data to transmit
         * @param length data length in bytes
         * @returns DUCK_ERR_NONE if the call was successful, an error code otherwise.
         */
        int startTransmitData(uint8_t* data, int length);

        /**
         * @brief Set the LoRa chip in standby mode.
         *
         * @returns DUCK_ERR_NONE if the chip is sucessfuly set in standby mode, an
         * error code otherwise.
         */
        int standBy();

        /**
         * @brief Set the LoRa radio into sleep mode.
         * 
         * @returns DUCK_ERR_NONE if the chip is sucessfuly set in standby mode, an
         * error code otherwise.   
         */
        int sleep();

        /*
        * @brief Interrupt service routine for the LoRa module.
        *
        */
        static void onInterrupt();

};

#endif