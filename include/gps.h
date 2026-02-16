#ifndef GPS_H
#define GPS_H

#include <Arduino.h>

namespace GPS
{
    void setup();
    void enableGPS();
    void disableGPS();

    char *ax25_base91enc(char *s, uint8_t n, uint32_t v);
    String encodeGPS(float latitude, float longitude, String overlay, String symbol);
    String generateBeacon(double latitude, double longitude, int speed, int direction, int altitude);
    double calculateDistanceCourse(double latitude, double longitude);
    String decodeEncodedGPS(String packet);
    String getReceivedGPS(String packet);
    String getDistance(String packet);

}

#endif