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

SoftwareSerial gpsPort(0, 1);
TinyGPSPlus gps;

Configuration   Config;
WiFiClient      espClient;

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

String iGateBeaconPacket, iGateLoRaBeaconPacket;

void setup() {
    Serial.begin(115200);

    gpsPort.begin(115200);

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
    Utils::validateFreqs();

    iGateBeaconPacket = GPS_Utils::generateBeacon();
    iGateLoRaBeaconPacket = GPS_Utils::generateiGateLoRaBeacon();

    WIFI_Utils::setup();
    WEB_Utils::setup();
}

int rxPackets = 0;

void loop() {
    while (gpsPort.available() > 0) {
        gps.encode(gpsPort.read());
    }

    Config.beacon.comment = "RX 436.050 @1k2 SKYY1-1 DIGI ";

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

    Config.beacon.comment += "S" + String(gps.satellites.value()) + " ";

    if (gps.altitude.isValid()) {
        Config.beacon.comment += "3D ";
    } else if (gps.location.isValid()) {
        Config.beacon.comment += "2D ";
    } else {
        if (gps.time.isValid()) {
            Config.beacon.comment += "T ";;
        }
    }

    Config.beacon.comment += "RXs " + String(rxPackets);

    if (gps.location.isValid()) {
        Config.beacon.latitude = gps.location.lat();
        Config.beacon.longitude = gps.location.lng();
    } else {
        Config.beacon.latitude = 0;
        Config.beacon.longitude = 0;
    }

    iGateBeaconPacket = GPS_Utils::generateBeacon();
    iGateLoRaBeaconPacket = GPS_Utils::generateiGateLoRaBeacon();

    WIFI_Utils::checkIfAutoAPShouldPowerOff();

    if (isUpdatingOTA) {
        ElegantOTA.loop();
        return;
    }

    #ifndef BALLOON
    WIFI_Utils::checkWiFi(); // Always use WiFi, not related to IGate/Digi mode
    // Utils::checkWiFiInterval();
    #endif

    Utils::checkBeaconInterval();

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