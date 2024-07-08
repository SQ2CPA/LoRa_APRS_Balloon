#include <WiFi.h>
#include "configuration.h"
#include "battery_utils.h"
#include "pins_config.h"
#include "wifi_utils.h"
#include "lora_utils.h"
#include "gps_utils.h"
#include "utils.h"

extern Configuration Config;
extern bool statusAfterBoot;
extern uint32_t lastBeaconTx;
extern bool beaconUpdate;
extern String beaconPacket;
extern int rssi;
extern float snr;
extern int freqError;
extern String distance;
extern uint32_t lastWiFiCheck;
extern bool WiFiConnect;
extern bool WiFiConnected;
extern int beaconNum;
extern int beaconFrequency;

namespace Utils
{

    void processStatus()
    {
        String e = Config.beacon.path;

        if (e != "")
            e = "," + e;

        String packet = Config.callsign + ">APLRG1" + e;

        if (statusAfterBoot && Config.beacon.sendViaRF)
        {
            delay(2000);

            packet += ":>LoRa APRS RX " + String(Config.loramodule.rxFreq, 3) + " @1k2 SKY1-1 DIGI";
            LoRa_Utils::changeFreq(434.855, "1200");
            LoRa_Utils::sendNewPacket(packet);
            LoRa_Utils::changeToRX();

            statusAfterBoot = false;
        }
    }

    bool checkBeaconInterval()
    {
        uint32_t lastTx = millis() - lastBeaconTx;
        String tBeaconPacket = beaconPacket;

        if (lastBeaconTx == 0 || lastTx >= 0.40 * 60 * 1000)
        { // ~25s
            beaconUpdate = true;
        }

        if (beaconUpdate)
        {
            Utils::println("-- Sending Beacon --");

            tBeaconPacket += Config.beacon.comment;

            // if (Config.sendBatteryVoltage)
            // {
            //     tBeaconPacket += " Batt=" + String(BATTERY_Utils::checkBattery(), 2) + "V";
            // }

            // if (Config.externalVoltageMeasurement)
            // {
            //     tBeaconPacket += " Ext=" + String(BATTERY_Utils::checkExternalVoltage(), 2) + "V";
            // }

            if (Config.beacon.sendViaRF)
            {
                switch (beaconFrequency)
                {
                case 1:
                    LoRa_Utils::changeFreq(434.855, "1200");
                    break;
                case 2:
                    LoRa_Utils::changeFreq(433.775, "300");
                    break;
                case 0:
                    LoRa_Utils::changeFreq(439.9125, "300");
                    break;
                }

                LoRa_Utils::sendNewPacket(tBeaconPacket);
                LoRa_Utils::changeToRX();
            }

            lastBeaconTx = millis();
            beaconUpdate = false;
            beaconNum++;
            beaconFrequency++;

            if (beaconFrequency >= 3)
                beaconFrequency = 0;

            return true;
        }

        if (statusAfterBoot)
            processStatus();

        return false;
    }

    void checkWiFiInterval()
    {
        uint32_t WiFiCheck = millis() - lastWiFiCheck;
        if (WiFi.status() != WL_CONNECTED && WiFiCheck >= 15 * 60 * 1000)
        {
            WiFiConnect = true;
        }
        if (WiFiConnect)
        {
            Serial.println("\nConnecting to WiFi ...");
            WIFI_Utils::startWiFi();
            lastWiFiCheck = millis();
            WiFiConnect = false;
        }
    }

    void print(String text)
    {
        Serial.print(text);
    }

    void println(String text)
    {
        Serial.println(text);
    }

}