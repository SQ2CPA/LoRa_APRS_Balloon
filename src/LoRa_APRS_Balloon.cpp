#include <Arduino.h>
#include <vector>
#include "configuration.h"
#include "pins_config.h"
#include "query_utils.h"
#include "lora_utils.h"
#include "digi_utils.h"
#include "gps_utils.h"
#include "utils.h"
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SparkFun_Ublox_Arduino_Library.h>
#include "historical_location.h"
#include "wspr.h"
#include "debug.h"
#include "current_day.h"
#include <Preferences.h>

#ifdef CONFIG_WSPR_ENABLE
SoftwareSerial gpsPort(1, 0); // reversed
#else
SoftwareSerial gpsPort(0, 1); // normal
#endif

TinyGPSPlus gps;
SFE_UBLOX_GPS sgps;

bool statusAfterBoot = true;
bool beaconUpdate = true;

uint32_t lastBeaconTx = 0;
uint32_t lastWSPRTx = 0;
uint32_t lastHistoricalLocationsTx = 0;

uint32_t lastHistoricalLocationsCheck = 0;
int historicalLocationsFrequency = 0;

#ifdef CONFIG_DEBUG_ENABLE
int beaconNum = 45;
#else
int beaconNum = 0;
#endif

String beaconPacket;
int beaconFrequency = 0;

int currentDay = -1;

bool canUseStorage = false;

Preferences preferences;

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

    delay(1000);

    LoRa_Utils::setup();

    Serial.print("Brownout: ");
    Serial.print(CONFIG_ESP32C3_BROWNOUT_DET);
    Serial.print(" ");
    Serial.print(CONFIG_ESP32C3_BROWNOUT_DET_LVL_SEL_7);
    Serial.print(" ");
    Serial.print(CONFIG_ESP32C3_BROWNOUT_DET_LVL);
    Serial.print("\n\n");

#ifdef CONFIG_GPS_FORWARD_TO_SERIAL
    while (true)
    {
        if (gpsPort.available() > 0)
        {
            byte b = gpsPort.read();

            Serial.write(b);
        }

        delay(250);
    }
#endif

    unsigned long startedAt = millis();

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

#ifdef CONFIG_DEBUG_ENABLE
    Historical_location::write("");
#endif

#ifdef CONFIG_WSPR_ENABLE
    WSPR_Utils::setup();

    if (WSPR_Utils::isAvailable())
        WSPR_Utils::disableTX();

        // WSPR_Utils::debug();
#endif

    if (preferences.begin("my-app", false))
    {
        canUseStorage = true;
        Serial.println("Preferences mounted successfully");
    }
    else
    {
        Serial.println("Preferences mount failed!!");
    }
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

String comment = "";
double latitude = 0, longitude = 0;

void loop()
{
    while (gpsPort.available() > 0)
    {
        byte data = gpsPort.read();

        gps.encode(data);
    }

    if (gpsFails >= 10)
    {
        sgps.begin(gpsPort);

        sgps.factoryReset();

        ESP.restart();
    }

    comment = "P" + String(beaconNum);
    comment += "S" + String(gps.satellites.value());
    comment += "F" + String(gpsFails);
    comment += "R" + String(rxPackets);

    if (outputPower < 20)
        comment += "O" + String(outputPower);

    if (currentDay > -1)
        comment += "D" + String(currentDay);

    comment += "N" + String(CONFIG_APRS_FLIGHT_ID);

    if (WSPR_Utils::isAvailable())
        comment += "W";

    if (canUseStorage)
        comment += "S";

    comment += " ";

#ifdef CONFIG_DEBUG_ENABLE
    gpsFails = 0;
#endif

    if (!gps.altitude.isValid())
    {
        if (gps.location.isValid())
        {
            comment += "2D ";
        }
        else if (gps.time.isValid())
        {
            if (gps.time.second() == 0 && gps.time.minute() == 0 && gps.time.hour() == 0)
            {
                comment += "T- ";
            }
            else
            {
                comment += "T+ ";
            }
        }
    }

#ifdef CONFIG_DEBUG_ENABLE
    comment += "DEBUG";
#endif

    int knots = 0;
    int course = 0;
    int altitude = 0;
    int altitudeInMeters = 0;

    if (gps.location.isValid())
    {
        latitude = gps.location.lat();
        longitude = gps.location.lng();

        knots = gps.speed.knots();
        course = gps.course.deg();
        altitude = gps.altitude.feet();
        altitudeInMeters = gps.altitude.meters();

        if (altitude < 0)
            altitude = 0;

        if (altitudeInMeters < 0)
            altitudeInMeters = 0;
    }
    else
    {
        latitude = 0;
        longitude = 0;
    }

#ifdef CONFIG_DEBUG_ENABLE
    beaconPacket = GPS_Utils::generateBeacon(0, 0, 0, 0, 0);

    altitude = 10000;

    DEBUG_Utils::setDummyLocation();
#else
    beaconPacket = GPS_Utils::generateBeacon(latitude, longitude, knots, course, altitude);
#endif

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

            if (gps.time.isValid())
            {
                Utils::print("Current time ");
                Utils::print(String(gps.time.hour()));
                Utils::print(":");
                Utils::print(String(gps.time.minute()));
                Utils::print(":");
                Utils::print(String(gps.time.second()));
                Utils::println("");
            }
        }
    }

    String packet = "";

#ifdef CONFIG_LORA_RX_ACTIVE
    packet = LoRa_Utils::receivePacket();
#endif

    if (packet != "")
    {
        rxPackets++;

        if (CONFIG_APRS_DIGI_MODE == 2)
        {
            DIGI_Utils::loop(packet);
        }
    }

#ifdef CONFIG_DEBUG_ENABLE
    altitudeInMeters = 4001;
#endif

#ifdef CONFIG_CURRENT_DAY_ENABLE
    if (beaconNum > 50 && altitudeInMeters > 2000 && canUseStorage)
    {
        Current_Day::read();

        Current_Day::setIfNotSet(gps.date.value());

        Current_Day::getDays(gps.date.value());
    }
#endif

#ifdef CONFIG_HISTORICAL_LOCATION_ENABLE
    if (beaconNum > 50 && altitudeInMeters > 2000 && canUseStorage)
    {
        if (!readHistoricalLocations)
        {
            lastHistoricalLocations = Historical_location::read();
            readHistoricalLocations = true;
        }

        uint32_t lastCheck = millis() - lastHistoricalLocationsCheck;

        if ((lastHistoricalLocationsCheck == 0 || lastCheck >= 15 * 1000) && latitude != 0 && longitude != 0)
        {
            lastHistoricalLocationsCheck = millis();

            Utils::println("Checking historical location");

            String location = Historical_location::makeLocationString();

            if (!setTodaysLocationSeparator)
            {
                Historical_location::setToday(location);
                setTodaysLocationSeparator = true;
            }

            if (int(latitude) != lastHistoricalLatitude || int(longitude) != lastHistoricalLongitude)
            {
                Historical_location::process(location);
            }
        }
    }

    if (lastHistoricalLocations != "")
        Historical_location::sendToRF();
#endif

#ifdef CONFIG_WSPR_ENABLE
    // WSPR DEBUG
    // latitude = 53.34449;
    // longitude = 17.642012;
    // altitude = 1250;

    if (WSPR_Utils::isAvailable() && (lastWSPRTx == 0 || millis() - lastWSPRTx >= 90 * 1000))
    {
        int minute = gps.time.minute();
        int second = gps.time.second();
        int centisecond = gps.time.centisecond();

        if (gps.time.isValid() && latitude != 0 && longitude != 0 && second != 0 && minute != 0 && gps.time.hour() != 0)
        {
            if (WSPR_Utils::isInTimeslot(minute, second))
            {
                Utils::println("Waiting for WSPR timeslot, time: " + String(minute) + ":" + String(second) + " " + String(centisecond));

                delay(60000 - (second * 1000 + centisecond * 10));

                delay(100);

                WSPR_Utils::prepareWSPR(altitudeInMeters);

                WSPR_Utils::sendWSPR(minute + 1);

                lastWSPRTx = millis();
            }
        }
    }
#endif
}