#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>

namespace Utils
{

    void sendStatus(String value);
    void sendDebug(String value);

    String getSender(const String &rawPacket);

    void checkTestInterval();
    bool checkBeaconInterval();

    void print(String text);
    void println(String text);

}

#endif