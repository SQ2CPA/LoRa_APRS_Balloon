#include <RadioLib.h>
#include <WiFi.h>
#include "configuration.h"
#include "pins_config.h"
#include "utils.h"

extern Configuration Config;

extern std::vector<ReceivedPacket> receivedPackets;

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

int rssi, freqError;
float snr;

namespace LoRa_Utils
{

    void setFlag(void)
    {
        operationDone = true;
    }

    void setup()
    {
        SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        int state = radio.begin(Config.loramodule.rxFreq);
        if (state == RADIOLIB_ERR_NONE)
        {
            Utils::println("Initializing LoRa Module");
        }
        else
        {
            Utils::println("Starting LoRa failed! Code: " + String(state));
            while (true)
                ;
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

        if (Config.loramodule.rxSpeed == "300")
        {
            radio.setSpreadingFactor(9);
            radio.setCodingRate(7);
            radio.setBandwidth(125);
        }
        else
        {
            radio.setSpreadingFactor(12);
            radio.setCodingRate(5);
            radio.setBandwidth(125);
        }
        radio.setCRC(true);

#if defined(RADIO_RXEN) && defined(RADIO_TXEN)
        radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
#endif

#if defined(HAS_SX126X) || ESP32C3_SX126X
        state = radio.setOutputPower(15); // max value 20dB for 400M30S as it has Low Noise Amp
#endif
#if defined(HAS_SX127X) || ESP32_DIY_1W_LoRa
        state = radio.setOutputPower(15); // max value 20dB for 400M30S as it has Low Noise Amp
#endif

        if (state == RADIOLIB_ERR_NONE)
        {
            Utils::println("init : LoRa Module    ...     done!");
        }
        else
        {
            Utils::println("Starting LoRa failed!");
            while (true)
                ;
        }
    }

    void setOutputPower(int power)
    {
        radio.setOutputPower(power);
    }

    void changeToRX()
    {
        if (Config.loramodule.rxSpeed == "300")
        {
            radio.setSpreadingFactor(12);
            radio.setCodingRate(5);
            radio.setBandwidth(125);
        }
        else
        {
            radio.setSpreadingFactor(9);
            radio.setCodingRate(7);
            radio.setBandwidth(125);
        }

        radio.setFrequency(Config.loramodule.rxFreq);
    }

    void changeFreq(float freq, String speed)
    {
        if (speed == "300")
        {
            radio.setSpreadingFactor(12);
            radio.setCodingRate(5);
            radio.setBandwidth(125);
        }
        else
        {
            radio.setSpreadingFactor(9);
            radio.setCodingRate(7);
            radio.setBandwidth(125);
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

    void sendNewPacket(uint8_t *newPacket, int size)
    {
        if (!Config.loramodule.txActive)
            return;

#ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin, HIGH);
#endif
        int state = radio.transmit(newPacket, size);
        transmissionFlag = true;
        if (state == RADIOLIB_ERR_NONE)
        {
            Utils::print("---> LoRa Packet Bin Tx: ");
            for (int i = 0; i < size; i++)
            {
                Serial.write(newPacket[i]);
            }
            Serial.print("\n");
            Utils::println(String(size));
        }
        else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
        {
            Utils::println(F("too long!"));
        }
        else if (state == RADIOLIB_ERR_TX_TIMEOUT)
        {
            Utils::println(F("timeout!"));
        }
        else
        {
            Utils::print(F("failed, code "));
            Utils::println(String(state));
        }
#ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin, LOW);
#endif
        // ignorePacket = true;
    }

    void sendNewPacket(const String &newPacket)
    {
        if (!Config.loramodule.txActive)
            return;

#ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin, HIGH);
#endif
        int state = radio.transmit("\x3c\xff\x01" + newPacket);
        transmissionFlag = true;
        if (state == RADIOLIB_ERR_NONE)
        {
            Utils::print("---> LoRa Packet Tx    : ");
            Utils::println(newPacket);
        }
        else if (state == RADIOLIB_ERR_PACKET_TOO_LONG)
        {
            Utils::println(F("too long!"));
        }
        else if (state == RADIOLIB_ERR_TX_TIMEOUT)
        {
            Utils::println(F("timeout!"));
        }
        else
        {
            Utils::print(F("failed, code "));
            Utils::println(String(state));
        }
#ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin, LOW);
#endif
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

        String loraPacket = "";

        if (transmissionFlag && !false)
        {
            radio.startReceive();
            transmissionFlag = false;
        }
        else
        {
            int state = radio.readData(loraPacket);
            if (state == RADIOLIB_ERR_NONE)
            {
                if (loraPacket != "" && !ignorePacket)
                {
                    rssi = radio.getRSSI();
                    snr = radio.getSNR();
                    freqError = radio.getFrequencyError();
                    Utils::println("<--- LoRa Packet Rx    : " + loraPacket);
                    Utils::println("(RSSI:" + String(rssi) + " / SNR:" + String(snr) + " / FreqErr:" + String(freqError) + ")");

                    if (!false)
                    {
                        ReceivedPacket receivedPacket;
                        receivedPacket.millis = millis();
                        receivedPacket.packet = loraPacket.substring(3);
                        receivedPacket.RSSI = rssi;
                        receivedPacket.SNR = snr;

                        if (receivedPackets.size() >= 20)
                        {
                            receivedPackets.erase(receivedPackets.begin());
                        }

                        receivedPackets.push_back(receivedPacket);
                    }

                    return loraPacket;
                }
            }
            else if (state == RADIOLIB_ERR_RX_TIMEOUT)
            {
                // timeout occurred while waiting for a packet
            }
            else if (state == RADIOLIB_ERR_CRC_MISMATCH)
            {
                Utils::println(F("CRC error!"));
                loraPacket = "";
            }
            else
            {
                Utils::print(F("failed, code "));
                Utils::println(String(state));
            }

            if (ignorePacket)
            {
                Utils::println("<--- LoRa Packet Rx    : " + loraPacket);
                Utils::println("Received own packet. Ignoring");

                ignorePacket = false;
                return "";
            }
        }
        return loraPacket;
    }
}