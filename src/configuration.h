#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include <Arduino.h>
#include <vector>
#include <FS.h>

class WiFi_AP
{
public:
    String ssid;
    String password;
};

class WiFi_Auto_AP
{
public:
    String password;
    int powerOff;
};

class Beacon
{
public:
    double latitude;
    double longitude;
    String comment;
    String overlay;
    String symbol;
    int interval;
    bool sendViaRF;
    String path;
};

class DIGI
{
public:
    int mode;
};

class LoraModule
{
public:
    double rxFreq;
    String rxSpeed;
    bool txActive;
    bool rxActive;
};

class OTA
{
public:
    String username;
    String password;
};

class WSPR
{
public:
    String callsign;
    int ssid;
};

class Configuration
{
public:
    String callsign;
    int rememberStationTime;
    bool sendBatteryVoltage;
    bool externalVoltageMeasurement;
    int externalVoltagePin;
    std::vector<WiFi_AP> wifiAPs;
    WiFi_Auto_AP wifiAutoAP;
    Beacon beacon;
    DIGI digi;
    LoraModule loramodule;
    OTA ota;
    WSPR wspr;

    void init();
    void writeFile();
    Configuration();

private:
    bool readFile();
    String _filePath;
};

#endif