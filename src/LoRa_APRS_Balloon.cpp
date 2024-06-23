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

SoftwareSerial gpsPort(0, 1);
TinyGPSPlus gps;
SFE_UBLOX_GPS sgps;

Configuration   Config;

uint8_t         myWiFiAPIndex         = 0;
int             myWiFiAPSize          = Config.wifiAPs.size();
WiFi_AP         *currentWiFi          = &Config.wifiAPs[myWiFiAPIndex];

bool            isUpdatingOTA         = false;
bool            statusAfterBoot       = true;
bool            beaconUpdate          = true;
uint32_t        lastBeaconTx          = 0;
uint32_t        previousWiFiMillis    = 0;

uint32_t        lastWiFiCheck         = 0;
bool            WiFiConnect           = true;
bool            WiFiConnected         = false;

bool            WiFiAutoAPStarted     = false;
long            WiFiAutoAPTime        = false;

uint32_t        lastBatteryCheck      = 0;

String          batteryVoltage;

std::vector<ReceivedPacket> receivedPackets;

String beaconPacket;
int beaconNum = 0;
int beaconFrequency = 0;

void setup() {
    Serial.begin(115200);

    gpsPort.begin(9600);

    #if defined(BATTERY_PIN)
    pinMode(BATTERY_PIN, INPUT);
    #endif
    #ifdef HAS_INTERNAL_LED
    pinMode(internalLedPin, OUTPUT);
    #endif

    if (Config.externalVoltageMeasurement) {
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
    Serial.print("\n");

    unsigned long startedAt = millis();

    // int d = 5000;
    int d = 15000;

    while (millis() - startedAt < d) {
        while (gpsPort.available() > 0) {
            byte b = gpsPort.read();

            if (millis() - startedAt < 3000) {
                Serial.write(b);
            }
            
            gps.encode(b);
        }

        delay(250);
    }
}

int rxPackets = 0;
int gpsFails = 0;

void loop() {
    while (gpsPort.available() > 0) {
        byte b = gpsPort.read();

        // Serial.write(b);
        gps.encode(b);
    }

    if (gpsFails >= 10) {
        sgps.begin(gpsPort);

        sgps.factoryReset();

        ESP.restart();
    }

    Config.beacon.comment = "P" + String(beaconNum);
    Config.beacon.comment += "S" + String(gps.satellites.value());
    Config.beacon.comment += " ";

    if (gps.altitude.isValid()) {
        Config.beacon.comment += String(int(gps.altitude.meters()))+"m ";
    } else {
        Config.beacon.comment += "-m ";
    }

    if (gps.speed.isValid()) {
        Config.beacon.comment += String(int(gps.speed.kmph()))+"kmh ";
    } else {
        Config.beacon.comment += "-kmh ";
    }

    if (gps.altitude.isValid()) {
        Config.beacon.comment += "3D ";
    } else if (gps.location.isValid()) {
        Config.beacon.comment += "2D ";
    } else {
        if (gps.time.isValid()) {
            Config.beacon.comment += "T ";;
        }
    }

    Config.beacon.comment += "R" + String(rxPackets);

    int knots = 0;
    int course = 0;
    int altitude = 0;

    if (gps.location.isValid()) {
        Config.beacon.latitude = gps.location.lat();
        Config.beacon.longitude = gps.location.lng();

        knots = gps.speed.knots();
        course = gps.course.deg();
        altitude = gps.altitude.feet();
    } else {
        Config.beacon.latitude = 0;
        Config.beacon.longitude = 0;
    }

    beaconPacket = GPS_Utils::generateBeacon(Config.beacon.latitude, Config.beacon.longitude, knots, course, altitude);

    // WIFI_Utils::checkIfAutoAPShouldPowerOff();

    if (isUpdatingOTA) {
        ElegantOTA.loop();
        return;
    }

    // WIFI_Utils::checkWiFi(); // Always use WiFi, not related to IGate/Digi mode
    // Utils::checkWiFiInterval();

    if (Utils::checkBeaconInterval()) {
        if (!gps.location.isValid()) {
            gpsFails++;
        } else {
            gpsFails = 0;
        }
    }

    String packet = "";
    if (Config.loramodule.rxActive) {
        packet = LoRa_Utils::receivePacket(); // We need to fetch LoRa packet above APRSIS and Digi
    } 

    if (packet != "") {
        rxPackets++;

        if (Config.digi.mode == 2) { // If Digi enabled
            DIGI_Utils::loop(packet); // Send received packet to Digi
        }
    }
}