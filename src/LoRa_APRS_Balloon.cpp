#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "configuration.h"
#include "pins_config.h"
#include "query_utils.h"
#include "lora_utils.h"
#include "wifi_utils.h"
#include "digi_utils.h"
#include "gps_utils.h"
#include "web_utils.h"
#include "utils.h"
#include <ElegantOTA.h>
#include "battery_utils.h"
#include <HWCDC.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SparkFun_Ublox_Arduino_Library.h>
#include "historical_location.h"
#include "wspr.h"

SoftwareSerial gpsPort(0, 1); // normal
// SoftwareSerial gpsPort(1, 0); // reversed
TinyGPSPlus gps;
SFE_UBLOX_GPS sgps;

Configuration Config;

uint8_t myWiFiAPIndex = 0;
int myWiFiAPSize = Config.wifiAPs.size();
WiFi_AP *currentWiFi = &Config.wifiAPs[myWiFiAPIndex];

bool isUpdatingOTA = false;
bool statusAfterBoot = true;
bool beaconUpdate = true;
uint32_t lastBeaconTx = 0;
uint32_t previousWiFiMillis = 0;

uint32_t lastWiFiCheck = 0;
bool WiFiConnect = true;
bool WiFiConnected = false;

bool WiFiAutoAPStarted = false;
long WiFiAutoAPTime = false;

uint32_t lastBatteryCheck = 0;

String batteryVoltage;

uint32_t lastHistoricalLocationsCheck = 0;
uint32_t lastHistoricalLocationsTx = 0;
int historicalLocationsFrequency = 0;

std::vector<ReceivedPacket> receivedPackets;

String beaconPacket;
int beaconNum = 0; // DEBUG ONLY 50
int beaconFrequency = 0;

void setup()
{
    Serial.begin(115200);

    gpsPort.begin(9600);

#if defined(BATTERY_PIN)
    pinMode(BATTERY_PIN, INPUT);
#endif
#ifdef HAS_INTERNAL_LED
    pinMode(internalLedPin, OUTPUT);
#endif

    if (Config.externalVoltageMeasurement)
    {
        pinMode(Config.externalVoltagePin, INPUT);
    }

    delay(1000);

    LoRa_Utils::setup();

    beaconPacket = GPS_Utils::generateBeacon(0, 0, 0, 0, 0);

    // WIFI_Utils::setup();
    // WEB_Utils::setup();

    Serial.print("Brownout: ");
    Serial.print(CONFIG_ESP32C3_BROWNOUT_DET);
    Serial.print(" ");
    Serial.print(CONFIG_ESP32C3_BROWNOUT_DET_LVL_SEL_7);
    Serial.print(" ");
    Serial.print(CONFIG_ESP32C3_BROWNOUT_DET_LVL);
    Serial.print("\n\n");

    unsigned long startedAt = millis();

    // int d = 5000;
    int d = 5000;

    while (millis() - startedAt < d)
    {
        while (gpsPort.available() > 0)
        {
            byte b = gpsPort.read();

            if (millis() - startedAt < 2000)
            {
                Serial.write(b);
            }

            gps.encode(b);
        }

        delay(250);
    }

    Serial.print("\n\n");

    // Historical_location::write(""); // DEBUG ONLY

    WSPR_Utils::setup();
}

int rxPackets = 0;
int gpsFails = 0;

String lastHistoricalLocations = "";

int lastHistoricalLatitude = 0, lastHistoricalLongitude = 0;

bool setTodaysLocationSeparator = false;
bool setFirstFullLocation = false;
bool readHistoricalLocations = false;

int okFrames = 0;
int outputPower = 15;

void loop()
{
    while (gpsPort.available() > 0)
    {
        byte b = gpsPort.read();

        // Serial.write(b);
        gps.encode(b);
    }

    if (gpsFails >= 10)
    {
        sgps.begin(gpsPort);

        sgps.factoryReset();

        ESP.restart();
    }

    Config.beacon.comment = "P" + String(beaconNum);
    Config.beacon.comment += "S" + String(gps.satellites.value());
    Config.beacon.comment += "F" + String(gpsFails);
    Config.beacon.comment += "R" + String(rxPackets);
    Config.beacon.comment += "O" + String(outputPower);
    Config.beacon.comment += "N21";
    Config.beacon.comment += " ";

    // gpsFails = 0; // DEBUG ONLY

    if (!gps.altitude.isValid())
    {
        if (gps.location.isValid())
        {
            Config.beacon.comment += "2D ";
        }
        else if (gps.time.isValid())
        {
            Config.beacon.comment += "T ";
        }
    }

    int knots = 0;
    int course = 0;
    int altitude = 0;

    if (gps.location.isValid())
    {
        Config.beacon.latitude = gps.location.lat();
        Config.beacon.longitude = gps.location.lng();

        knots = gps.speed.knots();
        course = gps.course.deg();
        altitude = gps.altitude.feet();

        if (altitude < 0)
            altitude = 0;
    }
    else
    {
        Config.beacon.latitude = 0;
        Config.beacon.longitude = 0;
    }

    beaconPacket = GPS_Utils::generateBeacon(Config.beacon.latitude, Config.beacon.longitude, knots, course, altitude);
    // beaconPacket = GPS_Utils::generateBeacon(0, 0, 0, 0, 0); // DEBUG ONLY

    // altitude = 10000; // DEBUG ONLY

    // WIFI_Utils::checkIfAutoAPShouldPowerOff();

    if (isUpdatingOTA)
    {
        ElegantOTA.loop();
        return;
    }

    // WIFI_Utils::checkWiFi(); // Always use WiFi, not related to IGate/Digi mode
    // Utils::checkWiFiInterval();

    if (Utils::checkBeaconInterval())
    {
        if (!gps.location.isValid())
        {
            gpsFails++;
        }
        else
        {
            gpsFails = 0;
        }

        okFrames++;

        if (okFrames >= 3 && outputPower < 19)
        {
            okFrames = 0;
            outputPower++;

            LoRa_Utils::setOutputPower(outputPower);

            Utils::println("Increase power to: " + String(outputPower));
        }
    }

    String packet = "";
    if (Config.loramodule.rxActive)
    {
        packet = LoRa_Utils::receivePacket(); // We need to fetch LoRa packet above APRSIS and Digi
    }

    if (packet != "")
    {
        rxPackets++;

        if (Config.digi.mode == 2)
        {                             // If Digi enabled
            DIGI_Utils::loop(packet); // Send received packet to Digi
        }
    }

    bool canWorkWithHistoricalLocation = beaconNum > 50 && altitude > 4000;

    if (canWorkWithHistoricalLocation)
    {
        if (!readHistoricalLocations)
        {
            lastHistoricalLocations = Historical_location::read();
            readHistoricalLocations = true;
        }

        uint32_t lastCheck = millis() - lastHistoricalLocationsCheck;

        if ((lastHistoricalLocationsCheck == 0 || lastCheck >= 15 * 1000) && Config.beacon.latitude != 0 && Config.beacon.longitude != 0)
        {
            Utils::println("Checking historical location");

            lastHistoricalLocationsCheck = millis();

            String location = Historical_location::makeLocationString();

            if (!setTodaysLocationSeparator)
            {
                if (lastHistoricalLocations != "")
                    lastHistoricalLocations += ";";

                lastHistoricalLocations += location;

                lastHistoricalLatitude = int(Config.beacon.latitude);
                lastHistoricalLongitude = int(Config.beacon.longitude);

                Utils::println("Inserted new day separator and first location for historical location");

                Historical_location::write(lastHistoricalLocations);

                Utils::print("Current historical locations: ");
                Utils::println(lastHistoricalLocations);

                setTodaysLocationSeparator = true;
            }

            if (int(Config.beacon.latitude) != lastHistoricalLatitude || int(Config.beacon.longitude) != lastHistoricalLongitude)
            {
                int diffLatitude = lastHistoricalLatitude - int(Config.beacon.latitude);
                int diffLongitude = lastHistoricalLongitude - int(Config.beacon.longitude);

                if (diffLatitude <= 1 && diffLatitude >= -1 && diffLongitude <= 1 && diffLongitude >= -1)
                {
                    String diffLatitudeS = Historical_location::makeDiff(diffLatitude);
                    String diffLongitudeS = Historical_location::makeDiff(diffLongitude);

                    if (diffLatitudeS != "00" || diffLongitudeS != "00")
                    {
                        lastHistoricalLocations += diffLatitudeS;
                        lastHistoricalLocations += diffLongitudeS;

                        Historical_location::write(lastHistoricalLocations);

                        Utils::print("Inserted new historical location: ");
                        Utils::println(location);

                        lastHistoricalLatitude = int(Config.beacon.latitude);
                        lastHistoricalLongitude = int(Config.beacon.longitude);
                    }
                }
                else
                {
                    Utils::print("Got too fast location change: ");
                    Utils::println(String(diffLatitude) + " " + String(diffLongitude));

                    lastHistoricalLatitude = int(Config.beacon.latitude);
                    lastHistoricalLongitude = int(Config.beacon.longitude);
                }

                Utils::print("Current historical locations: ");
                Utils::println(lastHistoricalLocations);
            }
        }
    }

    if (lastHistoricalLocations != "")
    {
        Historical_location::sendToRF();
    }

    if (gps.time.isValid() && Config.beacon.latitude != 0 && Config.beacon.longitude != 0)
    {
        // if (WSPR_Utils::isInTimeslot(gps.time.minute(), gps.time.second()))
        if (gps.time.minute() % 2 != 0)
        {
            Utils::println("Waiting for WSPR timeslot");

            delay(60000 - (gps.time.centisecond() * 10));

            WSPR_Utils::prepareWSPR(altitude);

            WSPR_Utils::sendWSPR();
        }
    }
}