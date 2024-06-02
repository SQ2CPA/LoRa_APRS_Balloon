#include <WiFi.h>
#include "configuration.h"
#include "query_utils.h"
#include "lora_utils.h"
#include "digi_utils.h"
#include "wifi_utils.h"
#include "gps_utils.h"
#include "utils.h"

extern Configuration    Config;
extern String           iGateBeaconPacket;


namespace DIGI_Utils {

    String generateDigiRepeatedPacket(String packet, String callsign) {
        String sender, temp0, tocall, path;
        sender = packet.substring(0, packet.indexOf(">"));
        temp0 = packet.substring(packet.indexOf(">") + 1, packet.indexOf(":"));
        if (temp0.indexOf(",") > 2) {
            tocall = temp0.substring(0, temp0.indexOf(","));
            path = temp0.substring(temp0.indexOf(",") + 1, temp0.indexOf(":"));
            if (path.indexOf("SKYY1-") >= 0) {
                String hop = path.substring(path.indexOf("SKYY1-") + 6, path.indexOf("SKYY1-") + 7);
                if (hop.toInt() >= 1 && hop.toInt() <= 7) {
                    if (hop.toInt() == 1) {
                        path.replace("SKYY1-1", callsign + "*");
                    }
                    else {
                        path.replace("SKYY1-" + hop, callsign + "*,SKYY1-" + String(hop.toInt() - 1));
                    }
                    path.replace("WIDE1-1", "WIDE1*");
                    path.replace("WIDE2-1", "WIDE2*");
                    path.replace("WIDE2-2", "WIDE2*");
                    String repeatedPacket = sender + ">" + tocall + "," + path + packet.substring(packet.indexOf(":"));
                    return repeatedPacket;
                }
                else {
                    return "";
                }
            }
            else {
                return "";
            }
        }
        else {
            return "";
        }
    }

    void processPacket(String packet) {
        String loraPacket, Sender, AddresseeAndMessage, Addressee;
        if (packet != "") {
            if ((packet.substring(0, 3) == "\x3c\xff\x01") && (packet.indexOf("NOGATE") == -1)) {
                Sender = packet.substring(3, packet.indexOf(">"));
                if (Sender != Config.callsign) {
                    AddresseeAndMessage = packet.substring(packet.indexOf("::") + 2);
                    Addressee = AddresseeAndMessage.substring(0, AddresseeAndMessage.indexOf(":"));
                    Addressee.trim();
                    if (packet.indexOf("SKYY1-") > 10 && Config.digi.mode == 2) { // If should repeat packet (SKYY1 Digi)
                        loraPacket = generateDigiRepeatedPacket(packet.substring(3), Config.callsign);
                        if (loraPacket != "") {
                            delay(500);
                            LoRa_Utils::sendNewPacket("APRS", loraPacket);
                        }
                    }
                }
            }
        }
    }

    void loop(String packet) {
        processPacket(packet);
    }

}