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
            // Initialize the random number generator with a seed based on the current time in a non-arduino way
            gen.seed(time(nullptr));
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
    private:
        static volatile uint16_t interruptFlags;
        static volatile bool receivedFlag;
        volatile bool isSetup = false;
        unsigned long lastReceiveTime = 0L;

        static void setReceiveFlag(bool value) { receivedFlag = value; }

        int goToReceiveMode(bool clear);
        int checkLoRaParameters(LoRaConfigParams config);

        void getSignalScore();
        void delay();
        std::mt19937 gen;
        static struct SignalInfo {
            float rssi;
            float snr;
            float signalScore;
        } signalInfo;

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

};

#endif