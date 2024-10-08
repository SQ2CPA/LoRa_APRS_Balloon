#include "historical_location.h"
#include "utils.h"
#include "configuration.h"
#include "lora_utils.h"
#include "pins_config.h"
#include <Preferences.h>

extern Preferences preferences;

extern double latitude;
extern double longitude;

extern int lastHistoricalLatitude;
extern int lastHistoricalLongitude;

extern uint32_t lastHistoricalLocationsTx;
extern int historicalLocationsFrequency;

extern String lastHistoricalLocations;

namespace Historical_location
{

    String read()
    {
        return preferences.getString("historical_location", "");
    }

    void write(String historicalLocations)
    {
        preferences.putString("historical_location", historicalLocations);
    }

    int encode(int arg1, int arg2, int arg3, int arg4)
    {
        arg1 += 1;
        arg2 += 1;
        arg3 += 1;
        arg4 += 1;

        int encoded = (arg1 * 27) + (arg2 * 9) + (arg3 * 3) + arg4;

        return encoded;
    }

    String makeDiff(int a)
    {
        String value = "00";

        if (a > 0)
            value = "+1";
        if (a < 0)
            value = "-1";

        return value;
    }

    String makeLocationString()
    {
        String location = "";

        int ilatitude = int(latitude);
        int ilongitude = int(longitude);

        // + + !
        // + - @
        // - + #
        // - - $

        String prefix = "%";

        if (ilatitude > 0 && ilongitude > 0)
            prefix = "!";
        else if (ilatitude > 0 && ilongitude < 0)
            prefix = "@";
        else if (ilatitude < 0 && ilongitude > 0)
            prefix = "#";
        else if (ilatitude < 0 && ilongitude < 0)
            prefix = "$";

        ilatitude = abs(ilatitude);
        ilongitude = abs(ilongitude);

        if (ilatitude < 10)
        {
            location += "0" + String(ilatitude);
        }
        else
        {
            location += String(ilatitude);
        }

        if (ilongitude < 10)
        {
            location += "00" + String(ilongitude);
        }
        else if (ilongitude < 100)
        {
            location += "0" + String(ilongitude);
        }
        else
        {
            location += String(ilongitude);
        }

        return prefix + location;
    }

    bool sendToRF()
    {
        uint32_t lastTx = millis() - lastHistoricalLocationsTx;

#ifdef CONFIG_DEBUG_ENABLE
        if (lastHistoricalLocationsTx == 0 || lastTx >= 1 * 60 * 1000) // 1.5 min
#else
        if (lastHistoricalLocationsTx == 0 || lastTx >= 10 * 60 * 1000)
#endif
        {
            String message = "";

            String historicalLocations = lastHistoricalLocations;

            while (historicalLocations != "")
            {
                int mode = -1;

                if (historicalLocations.startsWith(";"))
                {
                    mode = 0;
                }
                else
                {
                    String x = historicalLocations.substring(2, 4);

                    if (x.indexOf("+") != -1 || x.indexOf("-") != -1 || x.indexOf("00") != -1)
                    {
                        mode = 2;
                    }
                    else
                    {
                        mode = 1;
                    }
                }

                switch (mode)
                {
                case 0: // new day
                    message += " ";

                    historicalLocations = historicalLocations.substring(1);
                    break;
                case 1: // first location in day
                    message += historicalLocations.substring(0, 6);

                    historicalLocations = historicalLocations.substring(6);
                    break;
                case 2: // diff location
                    String aLatS = historicalLocations.substring(0, 2);
                    String aLngS = historicalLocations.substring(2, 4);

                    String x = historicalLocations.substring(4, 6);
                    bool hasNext = x.indexOf("+") != -1 || x.indexOf("-") != -1 || x.indexOf("00") != -1;

                    int a1 = 0;
                    int a2 = 0;

                    int b1 = 0;
                    int b2 = 0;

                    if (aLatS == "-1")
                    {
                        a1 = -1;
                    }
                    else if (aLatS == "+1")
                    {
                        a1 = 1;
                    }

                    if (aLngS == "-1")
                    {
                        a2 = -1;
                    }
                    else if (aLngS == "+1")
                    {
                        a2 = 1;
                    }

                    if (hasNext)
                    {
                        String bLatS = historicalLocations.substring(4, 6);
                        String bLngS = historicalLocations.substring(6, 8);

                        if (bLatS == "-1")
                        {
                            b1 = -1;
                        }
                        else if (bLatS == "+1")
                        {
                            b1 = 1;
                        }

                        if (bLngS == "-1")
                        {
                            b2 = -1;
                        }
                        else if (bLngS == "+1")
                        {
                            b2 = 1;
                        }

                        historicalLocations = historicalLocations.substring(8);
                    }
                    else
                    {
                        historicalLocations = historicalLocations.substring(4);
                    }

                    int value = encode(a1, a2, b1, b2);

                    char encoded = static_cast<char>('!' + value);

                    message += encoded;

                    break;
                }
            }

            if (message.length() > 220)
            {
                int i = lastHistoricalLocations.indexOf(";");

                lastHistoricalLocations = lastHistoricalLocations.substring(i + 1);
                write(lastHistoricalLocations);

                Utils::println("-- Removed oldest day from historical locations --");

                return false;
            }

            switch (historicalLocationsFrequency)
            {
            case 0:
                Utils::println("-- Sending Historical Locations [434.855] --");

                LoRa_Utils::changeFreq(434.855, "1200");
                break;
            case 1:
                Utils::println("-- Sending Historical Locations [433.775] --");

                LoRa_Utils::changeFreq(433.775, "300");
                break;
            case 2:
                Utils::println("-- Sending Historical Locations [439.9125] --");

                LoRa_Utils::changeFreq(439.9125, "300");
                break;
            }

            LoRa_Utils::sendNewPacket(String(CONFIG_APRS_CALLSIGN) + ">APLFLY,WIDE1*::SR2CPA-11:" + message);
            LoRa_Utils::changeToRX();

            lastHistoricalLocationsTx = millis();
            historicalLocationsFrequency++;

            if (historicalLocationsFrequency >= 3)
                historicalLocationsFrequency = 0;

            return true;
        }

        return false;
    }

    void setToday(String location)
    {
        if (lastHistoricalLocations != "")
            lastHistoricalLocations += ";";

        lastHistoricalLocations += location;

        lastHistoricalLatitude = int(latitude);
        lastHistoricalLongitude = int(longitude);

        Utils::println("Inserted new day separator and first location for historical location");

        write(lastHistoricalLocations);

        Utils::print("Current historical locations: ");
        Utils::println(lastHistoricalLocations);
    }

    void makeDiffAndInsert(int diffLatitude, int diffLongitude, String location)
    {
        String diffLatitudeS = Historical_location::makeDiff(diffLatitude);
        String diffLongitudeS = Historical_location::makeDiff(diffLongitude);

        if (diffLatitudeS != "00" || diffLongitudeS != "00")
        {
            lastHistoricalLocations += diffLatitudeS;
            lastHistoricalLocations += diffLongitudeS;

            write(lastHistoricalLocations);

            Utils::print("Inserted new historical location: ");
            Utils::print(location);
            Utils::print(" as ");

            Utils::print(diffLatitudeS);
            Utils::print(" ");
            Utils::print(diffLongitudeS);

            Utils::println("");
        }
    }

    void process(String location)
    {
        int diffLatitude = lastHistoricalLatitude - int(latitude);
        int diffLongitude = lastHistoricalLongitude - int(longitude);

        if (abs(diffLatitude) <= 1 && abs(diffLongitude) <= 1)
        {
            makeDiffAndInsert(diffLatitude, diffLongitude, location);

            lastHistoricalLatitude = int(latitude);
            lastHistoricalLongitude = int(longitude);

            Utils::print("Current historical locations: ");
            Utils::println(lastHistoricalLocations);
        }
        else
        {
            Utils::print("Got too fast location change: ");
            Utils::println(String(diffLatitude) + " " + String(diffLongitude));

            lastHistoricalLatitude = int(latitude);
            lastHistoricalLongitude = int(longitude);
        }
    }
}