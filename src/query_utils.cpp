#include "configuration.h"
#include "query_utils.h"

extern Configuration Config;

namespace QUERY_Utils
{

    String process(String query, String sender)
    {
        for (int i = sender.length(); i < 9; i++)
        {
            sender += ' ';
        }

        query.toUpperCase();

        String answer;
        if (query == "?APRS?" || query == "H" || query == "HELP" || query == "?")
        {
            answer = "?APRSV ?APRSP";
        }
        else if (query == "?APRSV")
        {
            answer = "LoRa APRS Balloon";
        }
        else if (query == "?APRSP")
        {
            answer = "Location: " + String(Config.beacon.latitude, 2) + " " + String(Config.beacon.longitude, 2);
        }

        if (Config.beacon.path == "")
        {
            return Config.callsign + ">APLRG1,RFONLY::" + sender + ":" + answer;
        }
        else
        {
            return Config.callsign + ">APLRG1," + Config.beacon.path + "::" + sender + ":" + answer;
        }
    }

}