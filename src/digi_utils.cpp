#include "configuration.h"
#include "query_utils.h"
#include "lora_utils.h"
#include "digi_utils.h"
#include "gps_utils.h"
#include "utils.h"

namespace DIGI_Utils
{

    String generateDigiRepeatedPacket(String packet, String callsign)
    {
        String sender, temp0, tocall, path;
        sender = packet.substring(0, packet.indexOf(">"));
        temp0 = packet.substring(packet.indexOf(">") + 1, packet.indexOf(":"));
        if (temp0.indexOf(",") > 2)
        {
            tocall = temp0.substring(0, temp0.indexOf(","));
            path = temp0.substring(temp0.indexOf(",") + 1, temp0.indexOf(":"));
            if (path.indexOf("SKY1-") >= 0)
            {
                String hop = path.substring(path.indexOf("SKY1-") + 5, path.indexOf("SKY1-") + 6);
                if (hop.toInt() >= 1 && hop.toInt() <= 7)
                {
                    if (hop.toInt() == 1)
                    {
                        path.replace("SKY1-1", callsign + "*");
                    }
                    else
                    {
                        path.replace("SKY1-" + hop, callsign + "*,SKY1-" + String(hop.toInt() - 1));
                    }
                    path.replace("WIDE1-1", "WIDE1*");
                    path.replace("WIDE2-1", "WIDE2*");
                    path.replace("WIDE2-2", "WIDE2*");
                    String repeatedPacket = sender + ">" + tocall + "," + path + packet.substring(packet.indexOf(":"));
                    return repeatedPacket;
                }
                else
                {
                    return "";
                }
            }
            else
            {
                return "";
            }
        }
        else
        {
            return "";
        }
    }

    void processReceivedLoRaMessage(String sender, String packet)
    {
        String receivedMessage;

        if (packet.indexOf("{") > 0)
        { // ack?
            String ackMessage = "ack" + packet.substring(packet.indexOf("{") + 1);
            ackMessage.trim();
            delay(4000);
            // Serial.println(ackMessage);
            for (int i = sender.length(); i < 9; i++)
            {
                sender += ' ';
            }

            LoRa_Utils::changeFreq(434.855, "1200");
            if (strcmp(CONFIG_APRS_PATH, ""))
            {
                LoRa_Utils::sendNewPacket(String(CONFIG_APRS_CALLSIGN) + ">APLRG1,RFONLY::" + sender + ":" + ackMessage);
            }
            else
            {
                LoRa_Utils::sendNewPacket(String(CONFIG_APRS_CALLSIGN) + ">APLRG1," + String(CONFIG_APRS_PATH) + "::" + sender + ":" + ackMessage);
            }
            LoRa_Utils::changeToRX();

            receivedMessage = packet.substring(packet.indexOf(":") + 1, packet.indexOf("{"));
        }
        else
        {
            receivedMessage = packet.substring(packet.indexOf(":") + 1);
        }

        if (receivedMessage.indexOf("?") == 0)
        {

            String result = QUERY_Utils::process(receivedMessage, sender);

            LoRa_Utils::changeFreq(434.855, "1200");
            LoRa_Utils::sendNewPacket(result);
            LoRa_Utils::changeToRX();
        }
    }

    void processPacket(String packet)
    {
        String loraPacket, Sender, AddresseeAndMessage, Addressee;
        if (packet != "")
        {
            if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("NOGATE") == -1))
            {
                Sender = packet.substring(3, packet.indexOf(">"));
                if (Sender != CONFIG_APRS_CALLSIGN)
                {
                    AddresseeAndMessage = packet.substring(packet.indexOf("::") + 2);
                    Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                    Addressee.trim();
                    if (packet.indexOf("::") > 10 && Addressee == CONFIG_APRS_CALLSIGN)
                    { // its a message for me!
                        processReceivedLoRaMessage(Sender, AddresseeAndMessage);
                    }
                    else if (packet.indexOf("SKY1-") > 10 && CONFIG_APRS_DIGI_MODE == 2)
                    { // If should repeat packet (SKY1 Digi)
                        loraPacket = generateDigiRepeatedPacket(packet.substring(3), CONFIG_APRS_CALLSIGN);
                        if (loraPacket != "")
                        {
                            delay(500);
                            LoRa_Utils::changeFreq(434.855, "1200");
                            LoRa_Utils::sendNewPacket(loraPacket);
                            LoRa_Utils::changeToRX();
                        }
                    }
                }
            }
        }
    }

    void loop(String packet)
    {
        processPacket(packet);
    }

}