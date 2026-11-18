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
            gen.seed(esp_random());
        };
         /*
        * @brief Interrupt service routine for the LoRa module.
        *
        */
       static void onInterrupt();
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
         * @brief Check whether the radio takes a TX power, without changing it.
         *
         * Runs the same checks as setupRadio(), including RadioLib's own, so a
         * value that passes here doesn't make radio setup fail. Of the 14 to 22
         * dBm that topics::cmd_tx allows, an SX1276 takes 14 to 17 or 20 and an
         * SX1262 takes all. Works before and after setupRadio().
         *
         * @param power TX power in dBm
         * @returns DUCK_ERR_NONE if the radio takes it, DUCK_ERR_INVALID_ARGUMENT otherwise.
         */
        int checkTxPower(int power);

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
        uint32_t getTxDoneCount() const { return txDoneCount; }
        uint32_t getTxTimeoutCount() const { return txTimeoutCount; }

        /**
         * @brief Check whether the radio still has work in progress.
         *
         * @return true while a transmission has not completed, a received packet
         * has not been read, or an interrupt has not been serviced yet.
         */
        bool isBusy() const { return transmitting || receivedFlag || interruptPending; }

        /**
         * @brief Set the LoRa radio into sleep mode.
         *
         * The radio stays asleep until it is set up again, normally at the next
         * boot.
         * @returns DUCK_ERR_NONE if the chip is successfully set in sleep mode, an
         * error code otherwise.
         */
        int sleep();

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

    private:
        // Set to true by onInterrupt() (ISR context) only. The actual IRQ flags
        // are read from the radio in loop context by serviceInterruptFlags().
        static volatile bool interruptPending;
        static volatile bool receivedFlag;
        volatile bool isSetup = false;
        bool transmitting = false;
        uint32_t txDoneCount = 0, txTimeoutCount = 0;
        uint32_t txStartedAt = 0, txTimeoutMs = 0;
        float lastRssi = 0, lastSnr = 0;
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

       

};

#endif
