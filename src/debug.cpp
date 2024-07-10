#include "configuration.h"
#include "pins_config.h"

extern Configuration Config;
extern int beaconNum;

namespace DEBUG_Utils
{
    void setDummyLocation()
    {
#ifdef DEBUG
        if (beaconNum > 75)
        {
            Config.beacon.latitude = 49.123;
            Config.beacon.longitude = 14.123;
        }
        else if (beaconNum > 70)
        {
            Config.beacon.latitude = 50.123;
            Config.beacon.longitude = 15.123;
        }
        else if (beaconNum > 65)
        {
            Config.beacon.latitude = 50.123;
            Config.beacon.longitude = 16.123;
        }
        else if (beaconNum > 60)
        {
            Config.beacon.latitude = 51.123;
            Config.beacon.longitude = 17.123;
        }
        else if (beaconNum > 55)
        {
            Config.beacon.latitude = 52.123;
            Config.beacon.longitude = 17.123;
        }
        else if (beaconNum > 50)
        {
            Config.beacon.latitude = 53.123;
            Config.beacon.longitude = 18.123;
        }
        else if (beaconNum > 45)
        {
            Config.beacon.latitude = 54.123;
            Config.beacon.longitude = 18.123;
        }
        else if (beaconNum > 40)
        {
            Config.beacon.latitude = 55.123;
            Config.beacon.longitude = 19.123;
        }
        else if (beaconNum > 35)
        {
            Config.beacon.latitude = 54.123;
            Config.beacon.longitude = 18.001;
        }
        else if (beaconNum > 30)
        {
            Config.beacon.latitude = 54.123;
            Config.beacon.longitude = 17.998;
        }
#endif
    }
}