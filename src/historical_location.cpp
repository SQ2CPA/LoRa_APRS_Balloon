#include "historical_location.h"
#include <SPIFFS.h>
#include "utils.h"
#include "configuration.h"
#include "lora_utils.h"

extern Configuration Config;

extern uint32_t lastHistoricalLocationsTx;
extern int historicalLocationsFrequency;

extern String lastHistoricalLocations;

namespace Historical_location
{

    String read()
    {
        File file = SPIFFS.open("/historical.txt", FILE_READ);

        if (!file)
        {
            return "";
        }

        String data = file.readString();

        file.close();

        return data;
    }

    void write(String historicalLocations)
    {
        File file = SPIFFS.open("/historical.txt", FILE_WRITE);
        if (!file)
        {
            return;
        }

        file.print(historicalLocations);
        file.close();
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

        int latitude = int(Config.beacon.latitude);
        int longitude = int(Config.beacon.longitude);

        if (latitude < 10)
        {
            location += "0" + String(latitude);
        }
        else
        {
            location += String(latitude);
        }

        if (longitude < 10)
        {
            location += "00" + String(longitude);
        }
        else if (longitude < 100)
        {
            location += "0" + String(longitude);
        }
        else
        {
            location += String(longitude);
        }

        return location;
    }

    bool sendToRF()
    {
        uint32_t lastTx = millis() - lastHistoricalLocationsTx;

        if (lastHistoricalLocationsTx == 0 || lastTx >= 10 * 60 * 1000)
        // if (lastHistoricalLocationsTx == 0 || lastTx >= 1.5 * 60 * 1000) // 1.5 min DEBUG ONLY
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
                    message += historicalLocations.substring(0, 5);

                    historicalLocations = historicalLocations.substring(5);
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

            if (message.length() > 200)
            {
                int i = lastHistoricalLocations.indexOf(" ");

                lastHistoricalLocations = lastHistoricalLocations.substring(i + 1);

                Utils::println("-- Removed oldest day from historical locations --");

                return false;
            }

            Utils::println("-- Sending Historical Locations --");

            if (Config.beacon.sendViaRF)
            {
                switch (historicalLocationsFrequency)
                {
                case 0:
                    LoRa_Utils::changeFreq(434.855, "1200");
                    break;
                case 1:
                    LoRa_Utils::changeFreq(433.775, "300");
                    break;
                case 2:
                    LoRa_Utils::changeFreq(439.9125, "300");
                    break;
                }

                LoRa_Utils::sendNewPacket(Config.callsign + ">APLRG1,WIDE1*::SR2CPA-11:" + message);
                LoRa_Utils::changeToRX();
            }

            lastHistoricalLocationsTx = millis();
            historicalLocationsFrequency++;

            if (historicalLocationsFrequency >= 3)
            {
                historicalLocationsFrequency = 0;
            }

            return true;
        }

        return false;
    }
}