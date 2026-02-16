#include "configuration.h"
#include "query.h"
#include "utils.h"
#include "radio.h"

extern double latitude;
extern double longitude;

extern int lastRSSI;
extern float lastSNR;

extern String lastReceivedPacket;

extern int altitudeInMeters;

namespace Query
{
    namespace
    {
        String buildHeader()
        {
            return String(CONFIG_APRS_CALLSIGN) + ">APLAIX";
        }

        void sendResponse(const String &sender, const String &message)
        {
            char processedStation[10];
            snprintf(processedStation, sizeof(processedStation), "%-9.9s", sender.c_str());

            String newPacket = buildHeader();
            newPacket += F("::");
            newPacket += processedStation;
            newPacket += ':';
            newPacket += message;

            RADIO::sendPacket(newPacket);
            RADIO::changeToRX();
        }
    }

    void sendMessage(String sender, String message)
    {
        sendResponse(sender, message);
    }

    void processQuery(String query, String sender)
    {
        query.toUpperCase();

        String answer;
        if (query == "?APRS?" || query == "H" || query == "HELP" || query == "?")
        {
            answer = "?APRSV ?APRSP ?APRSA ?APRST";
        }
        else if (query == "?APRSV")
        {
            answer = "LoRa APRS Balloon! www.SP0LND.pl/digi";
        }
        else if (query == "?APRSP" || query == "?APRSL")
        {
            answer = "Location: " + String(latitude, 4) + " " + String(longitude, 4);
        }
        else if (query == "?APRSA")
        {
            answer = "Altitude: " + String(altitudeInMeters) + "m";
        }
        else if (query == "?APRSGS")
        {
            answer = "RX speed: BW125k CR4:8 SF12";
        }
        else if (query == "?APRSGLP")
        {
            answer = "Last received packet: " + lastReceivedPacket;
        }
        else if (query == "?APRSLP")
        {
            answer = "Last packet: RSSI=" + String(lastRSSI) + " SNR=" + String(lastSNR, 1);
        }
        else if (query == "?APRST")
        {
            answer = "Internal temperature: " + String(temperatureRead(), 2) + " Celsius";
        }
        else
        {
            answer = "Unknown query command";
        }

        sendMessage(sender, answer);
    }

    bool process(const String &packet)
    {
        int pos = packet.indexOf(F("::"));
        if (pos == -1)
            return false;

        const String full = packet.substring(pos + 2);
        int colonPos = full.indexOf(':');
        if (colonPos == -1)
            return false;

        String target = full.substring(0, colonPos);
        target.trim();
        if (pos <= 10 || target != CONFIG_APRS_CALLSIGN)
            return false;

        const String sender = Utils::getSender(packet);
        String receivedMessage;

        delay(1500);

        int bracePos = packet.indexOf('{');
        if (bracePos > 0)
        {
            if (packet.indexOf("SKY2") != -1)
                RADIO::changeFreq(433.775, "300");
            else
                RADIO::changeFreq(434.855, "1200");

            String ackMessage = "ack";
            ackMessage += packet.substring(bracePos + 1);
            ackMessage.trim();

            sendResponse(sender, ackMessage);
            receivedMessage = full.substring(colonPos + 1, bracePos - (pos + 2));
        }
        else
        {
            receivedMessage = full.substring(colonPos + 1);
        }

        if (receivedMessage.startsWith("?"))
        {
            if (packet.indexOf("SKY2") != -1)
                RADIO::changeFreq(433.775, "300");
            else
                RADIO::changeFreq(434.855, "1200");

            processQuery(receivedMessage, sender);
        }

        return true;
    }

}