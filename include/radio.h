#ifndef RADIO_H
#define RADIO_H

#include <Arduino.h>

namespace RADIO
{

    void setup();

    void sendPacket(const String &newPacket);
    void sendPacket(uint8_t *newPacket, int size);

    String packetSanitization(String packet);
    String receivePacket();

    void changeFreq(float freq, int SF, int CR4, float BW);
    void changeFreq(float freq, String speed);
    void changeToRX();

    void startReceive();
    void setOutputPower(int power);

    float getRSSI();

}

#endif