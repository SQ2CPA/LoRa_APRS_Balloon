#include "configuration.h"
#include "query.h"
#include "radio.h"
#include "digi.h"
#include "gps.h"
#include "utils.h"

namespace Digi
{
    String build(const String &path, const String &original)
    {
        String packet = original;

        String newPath = path;

        if (path.indexOf(String(CONFIG_APRS_CALLSIGN)) != -1 && path.indexOf(String(CONFIG_APRS_CALLSIGN) + "*") == -1)
        {
            newPath.replace(String(CONFIG_APRS_CALLSIGN), String(CONFIG_APRS_CALLSIGN) + "*");
        }
        else if (path.indexOf("SKY3") != -1)
            newPath.replace("SKY3", String(CONFIG_APRS_CALLSIGN) + "*");
        else
        {
            if (path.indexOf("SKY1") != -1)
                newPath.replace("SKY1", String(CONFIG_APRS_CALLSIGN) + "*");

            if (path.indexOf("SKY2") != -1)
                newPath.replace("SKY2", String(CONFIG_APRS_CALLSIGN) + "*");
        }

        if (newPath == path)
            return "";

        return packet.substring(0, packet.indexOf(",") + 1) + newPath +
               packet.substring(packet.indexOf(":"));
    }

    String generate(const String &packet)
    {
        String path;
        String content = packet.substring(0, packet.indexOf(":"));

        int pathStart = content.indexOf(",");
        if (pathStart != -1)
            path = content.substring(pathStart + 1);

        if (path.indexOf(String(CONFIG_APRS_CALLSIGN)) != -1 && path.indexOf(String(CONFIG_APRS_CALLSIGN) + "*") == -1)
            return build(path, packet);

        if (path.indexOf("SKY3") != -1 && (path.startsWith("SKY3") || path.indexOf("*") != -1))
            return build(path, packet);

        if (path.indexOf("SKY1") != -1 && (path.startsWith("SKY1") || path.indexOf("*") != -1))
            return build(path, packet);

        if (path.indexOf("SKY2") != -1 && (path.startsWith("SKY2") || path.indexOf("*") != -1))
            return build(path, packet);

        return "";
    }

    void process(const String &rawPacket)
    {
        String sender = Utils::getSender(rawPacket);
        if (sender == "")
            return;

        if (sender == CONFIG_APRS_CALLSIGN)
            return;

        String digiPacket = generate(rawPacket);

        if (!digiPacket.isEmpty())
        {
            if (rawPacket.indexOf("SKY3") != -1 || rawPacket.indexOf(String(CONFIG_APRS_CALLSIGN)) != -1)
                RADIO::changeToRX();
            else if (rawPacket.indexOf("SKY1") != -1)
                RADIO::changeFreq(434.855, "1200");
            else if (rawPacket.indexOf("SKY2") != -1)
                RADIO::changeFreq(433.775, "300");

            delay(2000);

            RADIO::sendPacket(digiPacket);
            RADIO::changeToRX();
        }
    }
}