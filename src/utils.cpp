#include <WiFi.h>
#include "configuration.h"
#include "battery_utils.h"
#include "pins_config.h"
#include "wifi_utils.h"
#include "lora_utils.h"
#include "gps_utils.h"
#include "utils.h"

extern WiFiClient           espClient;
extern Configuration        Config;
extern bool                 statusAfterBoot;
extern uint32_t             lastBeaconTx;
extern bool                 beaconUpdate;
extern String               iGateBeaconPacket;
extern String               iGateLoRaBeaconPacket;
extern std::vector<String>  lastHeardStation;
extern int                  rssi;
extern float                snr;
extern int                  freqError;
extern String               distance;
extern uint32_t             lastWiFiCheck;
extern bool                 WiFiConnect;
extern bool                 WiFiConnected;


namespace Utils {

    void processStatus() {
        String status = Config.callsign + ">APLRG1," + Config.beacon.path;

        if (statusAfterBoot && Config.beacon.sendViaRF) {
            delay(2000);
            status += ":>LoRa APRS Balloon booted";
            LoRa_Utils::sendNewPacket("APRS", status);
            statusAfterBoot = false;
        }
    }

    String getLocalIP() {
        if (!WiFiConnected) {
            return "IP :  192.168.4.1";
        } else {
            return "IP :  " + String(WiFi.localIP()[0]) + "." + String(WiFi.localIP()[1]) + "." + String(WiFi.localIP()[2]) + "." + String(WiFi.localIP()[3]);
        }        
    }

    void setupDisplay() {
        #ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin,HIGH);
        #endif
        Serial.println("\nStarting Station: " + Config.callsign);
        #ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin,LOW);
        #endif
    }

    void checkBeaconInterval() {
        uint32_t lastTx = millis() - lastBeaconTx;
        String beaconPacket             = iGateBeaconPacket;
        String secondaryBeaconPacket    = iGateLoRaBeaconPacket;

        if (lastBeaconTx == 0 || lastTx >= Config.beacon.interval * 60 * 1000) {
            beaconUpdate = true;    
        }

        if (beaconUpdate) {
            Utils::println("-- Sending Beacon --");

            beaconPacket += Config.beacon.comment;
            secondaryBeaconPacket += Config.beacon.comment;

            #if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(HELTEC_HTCT62)
            if (Config.sendBatteryVoltage) {
                beaconPacket += " Batt=" + String(BATTERY_Utils::checkBattery(),2) + "V";
                secondaryBeaconPacket += " Batt=" + String(BATTERY_Utils::checkBattery(),2) + "V";
            }
            #endif

            if (Config.externalVoltageMeasurement) { 
                beaconPacket += " Ext=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V";
                secondaryBeaconPacket += " Ext=" + String(BATTERY_Utils::checkExternalVoltage(),2) + "V";
            }

            if (Config.beacon.sendViaRF) {
                LoRa_Utils::sendNewPacket("APRS", secondaryBeaconPacket);
            }

            lastBeaconTx = millis();
            beaconUpdate = false;
        }

        if (statusAfterBoot) {
            processStatus();
        }
    }

    void checkWiFiInterval() {
        uint32_t WiFiCheck = millis() - lastWiFiCheck;
        if (WiFi.status() != WL_CONNECTED && WiFiCheck >= 15*60*1000) {
        WiFiConnect = true;
        }
        if (WiFiConnect) {
        Serial.println("\nConnecting to WiFi ...");
        WIFI_Utils::startWiFi();
        lastWiFiCheck = millis();
        WiFiConnect = false;
        }
    }

    void validateFreqs() {
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq && abs(Config.loramodule.txFreq - Config.loramodule.rxFreq) < 125000) {
            Serial.println("Tx Freq less than 125kHz from Rx Freq ---> NOT VALID");
            Config.loramodule.txFreq = Config.loramodule.rxFreq; // Inform about that but then change the TX QRG to RX QRG and reset the device
            Config.writeFile();
            ESP.restart();
        }
    }

    void print(String text) {
        Serial.print(text);
    }

    void println(String text) {
        Serial.println(text);
    }

}