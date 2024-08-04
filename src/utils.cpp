#include "configuration.h"
#include "pins_config.h"
#include "lora_utils.h"
#include "gps_utils.h"
#include "utils.h"

extern bool statusAfterBoot;
extern uint32_t lastBeaconTx;
extern bool beaconUpdate;
extern String beaconPacket;
extern String comment;
extern int rssi;
extern float snr;
extern int freqError;
extern String distance;
extern int beaconNum;
extern int beaconFrequency;

namespace Utils
{

    void processStatus()
    {
        String e = CONFIG_APRS_PATH;

        if (e != "")
            e = "," + e;

        String packet = String(CONFIG_APRS_CALLSIGN) + ">APLRG1" + e;

        if (statusAfterBoot)
        {
            delay(2000);

            packet += ":>LoRa APRS RX " + String(CONFIG_LORA_RX_FREQ, 3) + " @1k2 SKY1-1 DIGI";
            LoRa_Utils::changeFreq(434.855, "1200");
            LoRa_Utils::sendNewPacket(packet);
            LoRa_Utils::changeToRX();

            statusAfterBoot = false;
        }
    }

    bool checkBeaconInterval()
    {
        uint32_t lastTx = millis() - lastBeaconTx;

        if (lastBeaconTx == 0 || lastTx >= 0.40 * 60 * 1000)
        { // ~25s
            beaconUpdate = true;
        }

        if (beaconUpdate)
        {
            switch (beaconFrequency)
            {
            case 1:
                Utils::println("-- Sending Beacon [434.855] --");

                LoRa_Utils::changeFreq(434.855, "1200");
                break;
            case 0:
                Utils::println("-- Sending Beacon [433.775] --");

                LoRa_Utils::changeFreq(433.775, "300");
                break;
            case 2:
                Utils::println("-- Sending Beacon [439.9125] --");

                LoRa_Utils::changeFreq(439.9125, "300");
                break;
            }

            LoRa_Utils::sendNewPacket(beaconPacket + comment);
            LoRa_Utils::changeToRX();

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

    void print(String text)
    {
        Serial.print(text);
    }

    void println(String text)
    {
        Serial.println(text);
    }

}