//
// Created by brent on 10/21/2025.
//
/**
 * @file SignalScore.h
 * @brief Contains the SignalScore structure for signal quality evaluation
 */
#ifndef CLUSTERDUCK_PROTOCOL_SIGNALSCORE_H
#define CLUSTERDUCK_PROTOCOL_SIGNALSCORE_H
/**
 * @struct SignalScore
 * @brief Structure to store signal quality metrics
 *
 * This struct contains signal quality measurements including RSSI (Received Signal Strength Indicator),
 * SNR (Signal-to-Noise Ratio), and a calculated overall signal score.
 */
struct SignalScore {
    /**
    * @brief Received Signal Strength Indicator in dBm
    *
    * Measures the power of the received radio signal. Typically ranges from
    * -120 (very weak) to -30 (very strong) for wireless communications.
    */
    float rssi;

    /**
     * @brief Signal-to-Noise Ratio in dB
     *
     * Measures the ratio of signal power to noise power. Higher values
     * indicate a cleaner signal with less noise interference.
     */
    float snr;

    /**
     * @brief Calculated overall signal quality score
     *
     * A derived metric combining RSSI and SNR to provide a single value
     * representing the overall quality of the signal connection.
     */
    float signalScore;
};
#endif //CLUSTERDUCK_PROTOCOL_SIGNALSCORE_H
