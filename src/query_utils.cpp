#include "configuration.h"
#include "query_utils.h"

extern Configuration        Config;


namespace QUERY_Utils {

    String process(String query, String station, String queryOrigin) {
        String answer;
        if (query=="?APRS?" || query=="?aprs?" || query=="?Aprs?" || query=="H" || query=="h" || query=="HELP" || query=="Help" || query=="help" || query=="?") {
            answer = "?APRSV ?APRSP ?APRSL ?APRSH ?WHERE callsign";
        } else if (query=="?APRSV" || query=="?aprsv" || query=="?Aprsv") {
            answer = "LoRa APRS Balloon";
        } else if (query=="?APRSP" || query=="?aprsp" || query=="?Aprsp") {
            answer = "Location: " + String(Config.beacon.latitude,2) + " " + String(Config.beacon.longitude,2);
        }
        for(int i = station.length(); i < 9; i++) {
            station += ' ';
        }
        if (queryOrigin == "APRSIS") {
            return Config.callsign + ">APLRG1,TCPIP,qAC::" + station + ":" + answer;// + "\n";
        } else { //} if (queryOrigin == "LoRa") {
            if (Config.beacon.path == "") {
                return Config.callsign + ">APLRG1,RFONLY::" + station + ":" + answer;
            } else {
                return Config.callsign + ">APLRG1,RFONLY," + Config.beacon.path + "::" + station + ":" + answer;
            }
        }
    }

}