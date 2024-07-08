#ifndef LORA_UTILS_H_
#define LORA_UTILS_H_

#include <Arduino.h>

namespace LoRa_Utils
{

    void setup();
    void sendNewPacket(const String &newPacket);
    void sendNewPacket(uint8_t *newPacket, int size);
    String packetSanitization(String packet);
    String receivePacket();
    void changeFreq(float freq, int SF, int CR4, float BW);
    void changeFreq(float freq, String speed);
    void changeToRX();
    void startReceive();

}

#endif