#include "configuration.h"
#include "pins_config.h"

extern int beaconNum;
extern double latitude;
extern double longitude;

namespace DEBUG_Utils
{
    void setDummyLocation()
    {
#ifdef CONFIG_DEBUG_ENABLE
        if (beaconNum > 95)
        {
            latitude = -49.123;
            longitude = -14.123;
        }
        else if (beaconNum > 90)
        {
            latitude = -50.123;
            longitude = -15.123;
        }
        else if (beaconNum > 85)
        {
            latitude = -50.123;
            longitude = -16.123;
        }
        else if (beaconNum > 80)
        {
            latitude = 51.123;
            longitude = 17.123;
        }
        else if (beaconNum > 75)
        {
            latitude = 52.123;
            longitude = 17.123;
        }
        else if (beaconNum > 70)
        {
            latitude = 53.123;
            longitude = 18.123;
        }
        else if (beaconNum > 65)
        {
            latitude = 54.123;
            longitude = 18.123;
        }
        else if (beaconNum > 60)
        {
            latitude = 55.123;
            longitude = 19.123;
        }
        else if (beaconNum > 55)
        {
            latitude = -54.123;
            longitude = -18.001;
        }
        else if (beaconNum > 50)
        {
            latitude = -54.123;
            longitude = -17.998;
        }
#endif
    }
}