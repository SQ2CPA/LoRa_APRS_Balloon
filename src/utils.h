#ifndef UTILS_H_
#define UTILS_H_

#include <Arduino.h>

namespace Utils
{

    void processStatus();
    bool checkBeaconInterval();
    void print(String text);
    void println(String text);

}

#endif