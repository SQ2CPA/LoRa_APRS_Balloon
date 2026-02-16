#include <Arduino.h>
#include <vector>
#include "configuration.h"
#include "pins_config.h"
#include "query.h"
#include "radio.h"
#include "digi.h"
#include "gps.h"
#include "utils.h"
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <SparkFun_Ublox_Arduino_Library.h>
#include "wspr.h"
#include <Preferences.h>
#include "esp_system.h"
#include "esp32/clk.h"
#include <WiFi.h>

SoftwareSerial gpsPort(CONFIG_GPS_PIN_TX, CONFIG_GPS_PIN_RX);

TinyGPSPlus gps;
SFE_UBLOX_GPS sgps;

uint32_t lastBeaconTx = 0;
uint32_t lastTestTx = 0;
uint32_t lastWSPRTx = 0;

int beaconNum = 0;

String beaconPacket;
int beaconFrequency = 0;

bool canUseStorage = false;

int outputPower = CONFIG_LORA_MIN_POWER;

Preferences preferences;

int restartReason = -1;

int rxPackets = 0;
int rxCRCPackets = 0;
int gpsFails = 0;

String comment = "";
double latitude = 0, longitude = 0;

int altitude = 0;
int altitudeInMeters = 0;

String lastReceivedPacket = "";

int lastRSSI = 0;
float lastSNR = 0;

int lastRSSIv = 0;
float lastSNRv = 0;

int beaconsWithoutRX = 0;

bool canIncrementSpeed = true;

void setup()
{
    WiFi.mode(WIFI_OFF);
    btStop();

    setCpuFrequencyMhz(40);

    Serial.begin(115200);

    gpsPort.begin(9600);

    delay(1000);

    RADIO::setup();

    Serial.print("[DEBUG] CPU Speed: ");
    Serial.print(esp_clk_cpu_freq());
    Serial.print(" ");
    Serial.print(esp_clk_apb_freq());
    Serial.print("\n");

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

    GPS::setup();
    GPS::enableGPS();

    int d = 10000;

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

#ifdef CONFIG_WSPR_ENABLE
    WSPR::setup();

    if (WSPR::isAvailable())
        WSPR::disableTX();

    // WSPR::debug();

#ifdef CONFIG_WSPR_STARTUP_TONE_ENABLE
    WSPR::startupTone();
#endif
#endif

    if (preferences.begin("my-app", false))
    {
        canUseStorage = true;
        Serial.println("[DEBG] Preferences mounted successfully");
    }
    else
    {
        Serial.println("[DEBG] Preferences mount failed!!");
    }

    restartReason = esp_reset_reason();

    Utils::println("[DEBUG] Started");

    Utils::sendStatus("LoRa APRS Balloon DIGI - www.SP0LND.pl/digi");
}

void loop()
{
    while (gpsPort.available() > 0)
    {
        byte data = gpsPort.read();

        gps.encode(data);
    }

#ifdef CONFIG_GPS_WATCHDOG
    if (gpsFails >= 10)
    {
        Serial.println("[DEBUG] No GPS, restarting!");

        sgps.begin(gpsPort);

        sgps.factoryReset();

        ESP.restart();
    }
#endif

    comment = "P" + String(beaconNum);
    comment += "S" + String(gps.satellites.value());
    comment += "F" + String(beaconFrequency + 1);
    comment += "O" + String(outputPower);

#ifdef CONFIG_APRS_FLIGHT_ID
    comment += "N" + String(CONFIG_APRS_FLIGHT_ID);
#endif

    comment += " ";

    if (WSPR::isAvailable())
        comment += "W";

    if (canUseStorage)
        comment += "S";

    comment += " ";

    comment += "R" + String(rxPackets);
    comment += "C" + String(rxCRCPackets);
    comment += "F" + String(gpsFails);
    comment += "Q" + String(restartReason);
    comment += "N" + String(beaconsWithoutRX);

    comment += " ";

    if (!gps.altitude.isValid())
    {
        if (gps.location.isValid())
            comment += "2D ";
        else if (gps.time.isValid())
        {
            if (gps.time.second() == 0 && gps.time.minute() == 0 && gps.time.hour() == 0)
                comment += "T- ";
            else
                comment += "T+ ";
        }
    }

    int knots = 0, course = 0;

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

    beaconPacket = GPS::generateBeacon(latitude, longitude, knots, course, altitude);

    if (Utils::checkBeaconInterval())
    {
        if (!gps.location.isValid())
            gpsFails++;
        else
            gpsFails = 0;

        outputPower = CONFIG_LORA_MIN_POWER + beaconNum / 3;

        if (outputPower > CONFIG_LORA_MAX_POWER)
            outputPower = CONFIG_LORA_MAX_POWER;

        RADIO::setOutputPower(outputPower);
    }

    Utils::checkTestInterval();

    String packet = "";

#ifdef CONFIG_LORA_RX_ACTIVE
    packet = RADIO::receivePacket();
#endif

    if (packet != "")
    {
        rxPackets++;

        if (!Query::process(packet))
        {
#ifdef CONFIG_APRS_DIGI_MODE
            Digi::process(packet);
#endif
        }

        beaconsWithoutRX = 0;

        lastReceivedPacket = packet;

        lastRSSI = lastRSSIv;
        lastSNR = lastSNRv;
    }

#ifdef CONFIG_WSPR_ENABLE
    // WSPR TEST
    // latitude = 53.34449;
    // longitude = 17.642012;
    // altitude = 1250;

    if (WSPR::isAvailable() && (lastWSPRTx == 0 || millis() - lastWSPRTx >= 90 * 1000))
    {
        int minute = gps.time.minute();
        int second = gps.time.second();
        int centisecond = gps.time.centisecond();

        if (gps.time.isValid() && second != 0 && minute != 0 && gps.time.hour() != 0)
        {
            if (WSPR::isInTimeslot(minute, second))
            {
                Utils::println("[WSPR] Waiting for WSPR timeslot, time: " + String(minute) + ":" + String(second) + " " + String(centisecond));

                delay(60000 - (second * 1000 + centisecond * 10));

                delay(100);

                WSPR::prepareWSPR(altitudeInMeters);

                WSPR::sendWSPR(minute + 1);

                lastWSPRTx = millis();
            }
        }
    }
#endif
}