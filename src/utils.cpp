#include "configuration.h"
#include "pins_config.h"
#include "radio.h"
#include "gps.h"
#include "utils.h"

extern uint32_t lastBeaconTx;
extern uint32_t lastTestTx;
extern bool beaconUpdate;
extern String beaconPacket;
extern String comment;
extern int rssi;
extern float snr;
extern int freqError;
extern String distance;
extern int beaconNum;
extern int beaconFrequency;
extern String lastReceivedPacket;

extern float lastSNR;
extern int lastRSSI;

namespace Utils
{
    void sendStatus(String value)
    {
        RADIO::changeFreq(434.855, "1200");
        RADIO::sendPacket(String(CONFIG_APRS_CALLSIGN) + ">APLAIX:>" + value);
        RADIO::changeFreq(433.775, "300");
        RADIO::sendPacket(String(CONFIG_APRS_CALLSIGN) + ">APLAIX:>" + value);

        RADIO::changeToRX();
    }

    String getSender(const String &rawPacket)
    {
        int colonBrace = rawPacket.indexOf(":}");

        bool fromAPRSIS = colonBrace != -1 && rawPacket.indexOf("TCPIP") != -1;

        if (fromAPRSIS)
        {
            int gtPos = rawPacket.indexOf('>', colonBrace + 2);

            if (gtPos != -1)
            {
                return rawPacket.substring(colonBrace + 2, gtPos);
            }

            return "";
        }

        int senderEnd = rawPacket.indexOf(">");
        if (senderEnd == -1)
            return "";

        return rawPacket.substring(0, senderEnd);
    }

    void sendDebug(String value)
    {
        RADIO::changeFreq(434.855, "1200");
        RADIO::sendPacket(String(CONFIG_APRS_CALLSIGN) + ">APLAIX::SR2CPA-11:" + String(lastRSSI) + "," + String(lastSNR, 1) + ":" + value);
        RADIO::changeToRX();
    }

    void checkTestInterval()
    {
        uint32_t lastTx = millis() - lastTestTx;

        if (lastTestTx == 0 || lastTx >= 4 * 60 * 1000)
        {
            sendDebug(lastReceivedPacket);

            lastTestTx = millis();
        }
    }

    bool checkBeaconInterval()
    {
        uint32_t lastTx = millis() - lastBeaconTx;

        if (lastBeaconTx == 0 || lastTx >= 60 * 1000)
        {
            comment += " ";

            comment += "RX=" + String(RADIO::getRSSI());

            switch (beaconFrequency)
            {
            case 1:
                Utils::println("[BEACON] Sending at 434.855");

                RADIO::changeFreq(434.855, "1200");
                break;
            case 0:
                Utils::println("[BEACON] Sending at 433.775");

                RADIO::changeFreq(433.775, "300");
                break;
            case 2:
                Utils::println("[BEACON] Sending at 439.9125");

                RADIO::changeFreq(439.9125, "300");
                break;
            }

            RADIO::sendPacket(beaconPacket + comment);
            RADIO::changeToRX();

            lastBeaconTx = millis();
            beaconNum++;
            beaconFrequency++;

            if (beaconFrequency >= 3)
                beaconFrequency = 0;

            return true;
        }

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