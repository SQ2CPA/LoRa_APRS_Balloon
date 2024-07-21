#include "configuration.h"
#include "query_utils.h"

extern double latitude;
extern double longitude;

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
            answer = "Location: " + String(latitude, 2) + " " + String(longitude, 2);
        }

        if (strcmp(CONFIG_APRS_PATH, ""))
        {
            return String(CONFIG_APRS_CALLSIGN) + ">APLRG1,RFONLY::" + sender + ":" + answer;
        }
        else
        {
            return String(CONFIG_APRS_CALLSIGN) + ">APLRG1," + String(CONFIG_APRS_PATH) + "::" + sender + ":" + answer;
        }
    }

}