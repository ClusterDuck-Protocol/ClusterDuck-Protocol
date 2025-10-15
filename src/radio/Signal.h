//
// Created by brent on 10/15/2025.
//

#ifndef CLUSTERDUCK_PROTOCOL_SIGNAL_H
#define CLUSTERDUCK_PROTOCOL_SIGNAL_H


class Signal {
public:
    Signal(){};
    Signal(float rssi, float snr, float signalScore): rssi(rssi), snr(snr), signalScore(signalScore) {};
    float rssi;
    float snr;
    float signalScore;

};


#endif //CLUSTERDUCK_PROTOCOL_SIGNAL_H
