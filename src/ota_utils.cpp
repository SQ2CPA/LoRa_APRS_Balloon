#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>
#include <AsyncTCP.h>
#include "configuration.h"
#include "ota_utils.h"

extern Configuration        Config;
extern bool                 isUpdatingOTA;

unsigned long ota_progress_millis = 0;


namespace OTA_Utils {
    
    void setup(AsyncWebServer *server) {
        if (Config.ota.username != ""  && Config.ota.password != "") {
            ElegantOTA.begin(server, Config.ota.username.c_str(), Config.ota.password.c_str());
        } else {
            ElegantOTA.begin(server);
        }
        
        ElegantOTA.setAutoReboot(true);
        ElegantOTA.onStart(onOTAStart);
        ElegantOTA.onProgress(onOTAProgress);
        ElegantOTA.onEnd(onOTAEnd);
    }

    void onOTAStart() {
        Serial.println("OTA update started!");
        isUpdatingOTA = true;
    }

    void onOTAProgress(size_t current, size_t final) {
        if (millis() - ota_progress_millis > 1000) {
            ota_progress_millis = millis();
            Serial.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
        }
    }

    void onOTAEnd(bool success) {
        if (success) {
            Serial.println("OTA update finished successfully!");
        } else {
            Serial.println("There was an error during OTA update!");
        }
        isUpdatingOTA = false;
    }
    
}