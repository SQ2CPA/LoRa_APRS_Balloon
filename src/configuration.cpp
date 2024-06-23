#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "configuration.h"

void Configuration::writeFile() {
    Serial.println("Saving config..");

    StaticJsonDocument<2048> data;
    File configFile = SPIFFS.open("/config.json", "w");

    if (wifiAPs[0].ssid != "") { // We don't want to save Auto AP empty SSID
        for (int i = 0; i < wifiAPs.size(); i++) {
            data["wifi"]["AP"][i]["ssid"] = wifiAPs[i].ssid;
            data["wifi"]["AP"][i]["password"] = wifiAPs[i].password;
            // data["wifi"]["AP"][i]["latitude"] = wifiAPs[i].latitude;
            // data["wifi"]["AP"][i]["longitude"] = wifiAPs[i].longitude;
        }
    }

    data["wifi"]["autoAP"]["password"] = wifiAutoAP.password;
    data["wifi"]["autoAP"]["powerOff"] = wifiAutoAP.powerOff;

    data["callsign"] = callsign;

    data["other"]["sendBatteryVoltage"] = sendBatteryVoltage;
    data["other"]["externalVoltageMeasurement"] = externalVoltageMeasurement;
    data["other"]["externalVoltagePin"] = externalVoltagePin;

    data["digi"]["mode"] = digi.mode;

    data["beacon"]["comment"] = beacon.comment;
    data["beacon"]["latitude"] = beacon.latitude;
    data["beacon"]["longitude"] = beacon.longitude;
    data["beacon"]["overlay"] = beacon.overlay;
    data["beacon"]["symbol"] = beacon.symbol;
    data["beacon"]["sendViaRF"] = beacon.sendViaRF;
    data["beacon"]["path"] = beacon.path;

    data["lora"]["rxFreq"] = loramodule.rxFreq;
    data["lora"]["power"] = loramodule.power;
    data["lora"]["txActive"] = loramodule.txActive;
    data["lora"]["rxActive"] = loramodule.rxActive;

    data["ota"]["username"] = ota.username;
    data["ota"]["password"] = ota.password;

    serializeJson(data, configFile);

    configFile.close();

    Serial.println("Config saved");
}

bool Configuration::readFile() {
    Serial.println("Reading config..");

    File configFile = SPIFFS.open("/config.json", "r");

    if (configFile) {
        StaticJsonDocument<2048> data;

        DeserializationError error = deserializeJson(data, configFile);
        if (error) {
            Serial.println("Failed to read file, using default configuration");
        }

        JsonArray WiFiArray = data["wifi"]["AP"];
        for (int i = 0; i < WiFiArray.size(); i++) {
            WiFi_AP wifiap;
            wifiap.ssid                   = WiFiArray[i]["ssid"].as<String>();
            wifiap.password               = WiFiArray[i]["password"].as<String>();

            wifiAPs.push_back(wifiap);
        }

        wifiAutoAP.password             = data["wifi"]["autoAP"]["password"].as<String>();
        wifiAutoAP.powerOff             = data["wifi"]["autoAP"]["powerOff"].as<int>();

        callsign                        = data["callsign"].as<String>();
        rememberStationTime             = data["other"]["rememberStationTime"].as<int>();
        sendBatteryVoltage              = data["other"]["sendBatteryVoltage"].as<bool>();
        externalVoltageMeasurement      = data["other"]["externalVoltageMeasurement"].as<bool>();
        externalVoltagePin              = data["other"]["externalVoltagePin"].as<int>();

        loramodule.power                = data["lora"]["power"].as<int>();

        ota.username                    = data["ota"]["username"].as<String>();
        ota.password                    = data["ota"]["password"].as<String>();

        beacon.latitude                   = data["beacon"]["latitude"].as<double>();
        beacon.longitude                  = data["beacon"]["longitude"].as<double>();
        beacon.comment                    = data["beacon"]["comment"].as<String>();
        beacon.overlay                    = data["beacon"]["overlay"].as<String>();
        beacon.symbol                     = data["beacon"]["symbol"].as<String>();
        beacon.sendViaRF                  = data["beacon"]["sendViaRF"].as<bool>();
        beacon.path                       = data["beacon"]["path"].as<String>();

        digi.mode                         = data["digi"]["mode"].as<int>();

        loramodule.rxFreq                 = data["lora"]["rxFreq"].as<long>();
        loramodule.txActive               = data["lora"]["txActive"].as<bool>();
        loramodule.rxActive               = data["lora"]["rxActive"].as<bool>();

        if (wifiAPs.size() == 0) { // If we don't have any WiFi's from config we need to add "empty" SSID for AUTO AP
            WiFi_AP wifiap;
            wifiap.ssid = "";
            wifiap.password = "";

            wifiAPs.push_back(wifiap);
        }
        configFile.close();
        Serial.println("Config read successfuly");
        return true;
    } else {
        Serial.println("Config file not found");
        return false;
    }
}
    
void Configuration::init() {
    WiFi_AP wifiap;
    wifiap.ssid                   = "";
    wifiap.password               = "";
    wifiAPs.push_back(wifiap);

    wifiAutoAP.password = "1234567890";
    wifiAutoAP.powerOff = 15;

    callsign = "N0CALL";

    beacon.comment = "LoRa APRS Balloon";
    beacon.latitude = 0.0;
    beacon.longitude = 0.0;
    beacon.overlay = "L";
    beacon.symbol = "#";
    beacon.sendViaRF = false;
    beacon.path = "WIDE1*";
    
    digi.mode = 0;

    loramodule.rxFreq = 433775000;
    loramodule.power = 20;
    loramodule.txActive = false;
    loramodule.rxActive = true;

    ota.username = "";
    ota.password = "";

    rememberStationTime = 30;
    sendBatteryVoltage = false;
    externalVoltageMeasurement = false;
    externalVoltagePin = 34;
}

Configuration::Configuration() {
    if (!SPIFFS.begin(false)) {
        Serial.println("SPIFFS Mount Failed");
        return;
    } else {
        Serial.println("SPIFFS Mounted");
    }

    bool exists = SPIFFS.exists("/config.json");
    if (!exists) {
        init();
        writeFile();
        ESP.restart();
    }

    readFile();
}