#include <RadioLib.h>
#include "configuration.h"
#include "pins_config.h"
#include "utils.h"

extern int rxCRCPackets;
extern String lastReceivedPacket;

extern int lastRSSIv;
extern float lastSNRv;

bool transmissionFlag = true;
bool ignorePacket = false;
bool operationDone = true;

#ifdef HAS_SX1262
SX1262 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

#ifdef HAS_SX1268
SX1268 radio = new Module(RADIO_CS_PIN, RADIO_DIO1_PIN, RADIO_RST_PIN, RADIO_BUSY_PIN);
#endif

#ifdef HAS_SX1278
SX1278 radio = new Module(RADIO_CS_PIN, RADIO_BUSY_PIN, RADIO_RST_PIN);
#endif

namespace RADIO
{

    void setFlag(void)
    {
        operationDone = true;
    }

    void startupTone()
    {
        int state = radio.beginFSK(433.025);
        if (state == RADIOLIB_ERR_NONE)
        {
            Utils::println("[RADIO] Initializing LoRa Module");
        }
        else
        {
            Utils::println("[RADIO] Starting LoRa failed! Code: " + String(state));
            delay(60000);
            ESP.restart();
        }

        for (int i = 0; i < 5; i++)
        {
            radio.transmit("HELLO FROM BALLOON www.SP0LND.pl");
            delay(125);
        }

        radio.reset();
    }

    void changeToRX()
    {
        radio.setSpreadingFactor(12);
        radio.setCodingRate(8);
        radio.setBandwidth(125);

        Serial.println("[RADIO] Change frequency to RX: " + String(CONFIG_LORA_RX_FREQ, 4) + " MHz SF12 CR4:8");

        radio.setFrequency(CONFIG_LORA_RX_FREQ);
    }

    void setup()
    {
        SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);

#ifdef CONFIG_LORA_STARTUP_TONE_ENABLE
        startupTone();
#endif

        int state = radio.begin(CONFIG_LORA_RX_FREQ);
        if (state == RADIOLIB_ERR_NONE)
        {
            Utils::println("[RADIO] Initializing LoRa Module");
        }
        else
        {
            Utils::println("[RADIO] Starting LoRa failed! Code: " + String(state));
            delay(60000);
            ESP.restart();
        }

#ifdef HAS_SX127X
        radio.setDio0Action(setFlag, RISING);
#endif
#ifdef HAS_SX126X
        if (!false)
        {
            radio.setDio1Action(setFlag);
        }
        else
        {
            radio.setDIOMapping(1, RADIOLIB_SX126X_IRQ_RX_DONE);
        }
#endif

        changeToRX();

        radio.setCRC(true);

        state = radio.setOutputPower(15);

        if (state == RADIOLIB_ERR_NONE)
        {
            Utils::println("[RADIO] LoRa initialized");
        }
        else
        {
            Utils::println("[RADIO] Starting LoRa failed!");
            delay(60000);
            ESP.restart();
        }
    }

    void setOutputPower(int power)
    {
        radio.setOutputPower(power);
    }

    void changeFreq(float freq, String speed)
    {
        if (speed == "300")
        {
            radio.setSpreadingFactor(12);
            radio.setCodingRate(5);
            radio.setBandwidth(125);

            Serial.println("[RADIO] Change frequency to: " + String(freq, 4) + " MHz speed 300bps");
        }
        else
        {
            radio.setSpreadingFactor(9);
            radio.setCodingRate(7);
            radio.setBandwidth(125);

            Serial.println("[RADIO] Change frequency to: " + String(freq, 4) + " MHz speed 1200bps");
        }

        radio.setFrequency(freq);
    }

    void changeFreq(float freq, int SF, int CR4, float BW)
    {
        radio.setSpreadingFactor(SF);
        radio.setBandwidth(BW);
        radio.setCodingRate(CR4);
        radio.setFrequency(freq);
    }

    void sendPacket(uint8_t *newPacket, int size)
    {
        int state = radio.transmit(newPacket, size);
        transmissionFlag = true;
        if (state == RADIOLIB_ERR_NONE)
        {
            Utils::print("[RADIO] TX (bin): ");
            for (int i = 0; i < size; i++)
            {
                Serial.write(newPacket[i]);
            }
            Serial.print("\n");
            Utils::println(String(size));
        }
        else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
        {
            Utils::println(F("[RADIO] too long!"));
        }
        else if (state == RADIOLIB_ERR_TX_TIMEOUT)
        {
            Utils::println(F("[RADIO] timeout!"));
        }
        else
        {
            Utils::print(F("[RADIO] failed, code "));
            Utils::println(String(state));
        }

        // ignorePacket = true;
    }

    void sendPacket(const String &newPacket)
    {
        int state = radio.transmit("\x3c\xff\x01" + newPacket);
        transmissionFlag = true;
        if (state == RADIOLIB_ERR_NONE)
        {
            Utils::print("[RADIO] TX: ");
            Utils::println(newPacket);
        }
        else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
        {
            Utils::println(F("[RADIO] too long!"));
        }
        else if (state == RADIOLIB_ERR_TX_TIMEOUT)
        {
            Utils::println(F("[RADIO] timeout!"));
        }
        else
        {
            Utils::print(F("[RADIO] failed, code "));
            Utils::println(String(state));
        }

        // ignorePacket = true;
    }

    String packetSanitization(String packet)
    {
        if (packet.indexOf("\0") > 0)
        {
            packet.replace("\0", "");
        }
        if (packet.indexOf("\r") > 0)
        {
            packet.replace("\r", "");
        }
        if (packet.indexOf("\n") > 0)
        {
            packet.replace("\n", "");
        }
        return packet;
    }

    void startReceive()
    {
        radio.startReceive();
    }

    String receivePacket()
    {
        if (!operationDone && !false)
            return "";

        operationDone = false;

        String packet = "";

        if (transmissionFlag && !false)
        {
            radio.startReceive();
            transmissionFlag = false;
        }
        else
        {
            int state = radio.readData(packet);
            if (state == RADIOLIB_ERR_NONE)
            {
                if (packet != "" && !ignorePacket)
                {
                    lastRSSIv = radio.getRSSI();
                    lastSNRv = radio.getSNR();

                    int freqError = radio.getFrequencyError();

                    Utils::println("[RADIO] RX: " + packet);
                    Utils::println("[RADIO] RSSI:" + String(lastRSSIv) + " / SNR:" + String(lastSNRv) + " / FreqErr:" + String(freqError) + "");

                    return packet.substring(3);
                }
            }
            else if (state == RADIOLIB_ERR_RX_TIMEOUT)
            {
                // timeout occurred while waiting for a packet
            }
            else if (state == RADIOLIB_ERR_CRC_MISMATCH)
            {
                rxCRCPackets++;

                lastReceivedPacket = "CRC:" + packet.substring(3);

                lastRSSIv = radio.getRSSI();
                lastSNRv = radio.getSNR();

                Utils::println(F("[RADIO] CRC error!"));
                packet = "";
            }
            else
            {
                Utils::print(F("[RADIO] failed, code "));
                Utils::println(String(state));
            }

            if (ignorePacket)
            {
                Utils::println("[RADIO] LoRa Packet Rx: " + packet);
                Utils::println("[RADIO] Received own packet. Ignoring");

                ignorePacket = false;
                return "";
            }
        }
        return packet;
    }

    float getRSSI()
    {
        return radio.getRSSI(false);
    }
}