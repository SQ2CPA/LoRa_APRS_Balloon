#ifndef CURRENT_DAY_H_
#define CURRENT_DAY_H_

#include <Arduino.h>

namespace Current_Day
{

    int read();
    void write(int day);
}

#endif