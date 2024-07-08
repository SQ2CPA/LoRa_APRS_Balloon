#ifndef HISTORICAL_LOCATION_H_
#define HISTORICAL_LOCATION_H_

#include <Arduino.h>

namespace Historical_location
{

    String read();
    void write(String historicalLocations);
    bool sendToRF();
    String makeDiff(int a);
    String makeLocationString();

}

#endif