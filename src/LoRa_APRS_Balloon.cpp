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
#include "debug.h"

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

#ifdef DEBUG
int beaconNum = 50;
#else
int beaconNum = 0;
#endif

String beaconPacket;
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

#ifdef DEBUG
    Historical_location::write("");
#endif

#ifdef WSPR
    WSPR_Utils::setup();

    if (WSPR_Utils::isAvailable())
        WSPR_Utils::disableTX();
#endif
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

int currentDay = -1;

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

    if (outputPower < 20)
        Config.beacon.comment += "O" + String(outputPower);

    if (currentDay != -1)
        Config.beacon.comment += "D" + String(currentDay);

    Config.beacon.comment += "N22";

    if (WSPR_Utils::isAvailable())
        Config.beacon.comment += "W";

    Config.beacon.comment += " ";

#ifdef DEBUG
    gpsFails = 0;
#endif

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

#ifdef DEBUG
    Config.beacon.comment += "DEBUG";
#endif

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

#ifdef DEBUG
    beaconPacket = GPS_Utils::generateBeacon(0, 0, 0, 0, 0);

    altitude = 10000;

    DEBUG_Utils::setDummyLocation();
#else
    beaconPacket = GPS_Utils::generateBeacon(Config.beacon.latitude, Config.beacon.longitude, knots, course, altitude);
#endif

    // WIFI_Utils::checkIfAutoAPShouldPowerOff();

    if (isUpdatingOTA)
    {
        ElegantOTA.loop();
        return;
    }

    // WIFI_Utils::checkWiFi();
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

        if (okFrames >= 3 && outputPower < 20)
        {
            okFrames = 0;
            outputPower++;

            LoRa_Utils::setOutputPower(outputPower);

            Utils::println("Increased power to: " + String(outputPower));
        }
    }

    String packet = "";
    if (Config.loramodule.rxActive)
    {
        packet = LoRa_Utils::receivePacket();
    }

    if (packet != "")
    {
        rxPackets++;

        if (Config.digi.mode == 2)
        {
            DIGI_Utils::loop(packet);
        }
    }

    bool canWorkWithHistoricalLocation = beaconNum > 50 && altitude > 4000;

    if (canWorkWithHistoricalLocation)
    {
        if (!readHistoricalLocations)
        {
            lastHistoricalLocations = Historical_location::read();
            readHistoricalLocations = true;

            for (int i = 0; i < lastHistoricalLocations.length(); i++)
            {
                if (lastHistoricalLocations.charAt(i) == ' ')
                    currentDay++;
            }
        }

        uint32_t lastCheck = millis() - lastHistoricalLocationsCheck;

        if ((lastHistoricalLocationsCheck == 0 || lastCheck >= 15 * 1000) && Config.beacon.latitude != 0 && Config.beacon.longitude != 0)
        {
            lastHistoricalLocationsCheck = millis();

            Utils::println("Checking historical location");

            String location = Historical_location::makeLocationString();

            if (!setTodaysLocationSeparator)
            {
                Historical_location::setToday(location);
                setTodaysLocationSeparator = true;
            }

            if (int(Config.beacon.latitude) != lastHistoricalLatitude || int(Config.beacon.longitude) != lastHistoricalLongitude)
            {
                Historical_location::process(location);
            }
        }
    }

    if (lastHistoricalLocations != "")
    {
        Historical_location::sendToRF();
    }

#ifdef WSPR
    if (WSPR_Utils::isAvailable())
    {
        if (gps.time.isValid() && Config.beacon.latitude != 0 && Config.beacon.longitude != 0)
        {
            // if (WSPR_Utils::isInTimeslot(gps.time.minute(), gps.time.second()))
            if (gps.time.minute() % 2 != 0)
            {
                Utils::println("Waiting for WSPR timeslot");

                delay(60000 - (gps.time.centisecond() * 10));

                WSPR_Utils::prepareWSPR(altitude);

                int txMode = 1;

                if (gps.time.minute() == 30)
                    txMode = 0;

                WSPR_Utils::sendWSPR(txMode);
            }
        }
    }
#endif
}