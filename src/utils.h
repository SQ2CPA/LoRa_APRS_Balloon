#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>

class ReceivedPacket {
public:
    long    millis;
    String  packet;
    int     RSSI;
    float     SNR;
};

namespace Utils {

    void processStatus();
    String getLocalIP();
    void setupDisplay();
    void checkBeaconInterval();
    void checkWiFiInterval();
    void validateFreqs();
    void print(String text);
    void println(String text);

}

#endif