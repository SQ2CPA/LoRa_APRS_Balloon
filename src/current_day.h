#ifndef CURRENT_DAY_H_
#define CURRENT_DAY_H_

#include <Arduino.h>

namespace Current_Day
{

    void read();
    void write(int day);
    void setIfNotSet(uint32_t date);
    void getDays(uint32_t date);
}

#endif