#include <RadioLib.h>
#include <WiFi.h>
#include "configuration.h"
#include "pins_config.h"
#include "utils.h"

extern Configuration  Config;

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

namespace LoRa_Utils {

    void setFlag(void) {
        operationDone = true;
    }

    void setup() {
        SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN);
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        int state = radio.begin(freq);
        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("Initializing LoRa Module");
        } else {
            Utils::println("Starting LoRa failed! Code: " + String(state));
            while (true);
        }
        #ifdef HAS_SX127X        
        radio.setDio0Action(setFlag, RISING);
        #endif
        #ifdef HAS_SX126X
        if (false) { // low power
            radio.setDio1Action(setFlag);
        } else {
            radio.setDIOMapping(1, RADIOLIB_SX126X_IRQ_RX_DONE);
        }
        #endif
        radio.setSpreadingFactor(Config.loramodule.spreadingFactor);
        float signalBandwidth = Config.loramodule.signalBandwidth/1000;
        radio.setBandwidth(signalBandwidth);
        radio.setCodingRate(Config.loramodule.codingRate4);
        radio.setCRC(true);

        #if defined(RADIO_RXEN) && defined(RADIO_TXEN)
        radio.setRfSwitchPins(RADIO_RXEN, RADIO_TXEN);
        #endif

        #if defined(HAS_SX126X) || ESP32C3_SX126X
        state = radio.setOutputPower(Config.loramodule.power); // max value 20dB for 400M30S as it has Low Noise Amp
        #endif
        #if defined(HAS_SX127X) || ESP32_DIY_1W_LoRa
        state = radio.setOutputPower(Config.loramodule.power); // max value 20dB for 400M30S as it has Low Noise Amp
        #endif   
        #if defined(HELTEC_V3)  || defined(HELTEC_WS) || defined(TTGO_T_Beam_V1_0_SX1268) || defined(TTGO_T_Beam_V1_2_SX1262)
        state = radio.setOutputPower(Config.loramodule.power + 2); // values available: 10, 17, 22 --> if 20 in tracker_conf.json it will be updated to 22.
        #endif
        if (state == RADIOLIB_ERR_NONE) {
            Utils::println("init : LoRa Module    ...     done!");
        } else {
            Utils::println("Starting LoRa failed!");
            while (true);
        }
    }

    void changeFreqTx() {
        delay(500);
        float freq = (float)Config.loramodule.txFreq / 1000000;
        radio.setFrequency(freq);
    }

    void changeFreqRx() {
        delay(500);
        float freq = (float)Config.loramodule.rxFreq / 1000000;
        radio.setFrequency(freq);
    }

    void sendNewPacket(const String& typeOfMessage, const String& newPacket) {
        if (!Config.loramodule.txActive) return;

        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            changeFreqTx();
        }

        #ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin, HIGH);
        #endif
        int state = radio.transmit("\x3c\xff\x01" + newPacket);
        transmissionFlag = true;
        if (state == RADIOLIB_ERR_NONE) {
            Utils::print("---> LoRa Packet Tx    : ");
            Utils::println(newPacket);
        } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) {
            Utils::println(F("too long!"));
        } else if (state == RADIOLIB_ERR_TX_TIMEOUT) {
            Utils::println(F("timeout!"));
        } else {
            Utils::print(F("failed, code "));
            Utils::println(String(state));
        }
        #ifdef HAS_INTERNAL_LED
        digitalWrite(internalLedPin, LOW);
        #endif
        if (Config.loramodule.txFreq != Config.loramodule.rxFreq) {
            changeFreqRx();
        }
        //ignorePacket = true;
    }

    String generatePacket(String aprsisPacket) {
        String firstPart, messagePart;
        aprsisPacket.trim();
        firstPart = aprsisPacket.substring(0, aprsisPacket.indexOf(","));
        messagePart = aprsisPacket.substring(aprsisPacket.indexOf("::") + 2);
        return firstPart + ",TCPIP,WIDE1-1," + Config.callsign + "::" + messagePart;
    }

    String packetSanitization(String packet) {
        if (packet.indexOf("\0") > 0) {
            packet.replace("\0", "");
        }
        if (packet.indexOf("\r") > 0) {
            packet.replace("\r", "");
        }
        if (packet.indexOf("\n") > 0) {
            packet.replace("\n", "");
        }
        return packet;
    }

    void startReceive() {
        radio.startReceive();
    }

    String receivePacket() {
        if(!operationDone && true) return ""; // Low power
        
        operationDone = false;

        String loraPacket = "";

        if (transmissionFlag && true) { // Low power
            radio.startReceive();
            transmissionFlag = false;
        } else {
            int state = radio.readData(loraPacket);
            if (state == RADIOLIB_ERR_NONE) {
                if (loraPacket != "" && !ignorePacket) {
                    rssi = radio.getRSSI();
                    snr = radio.getSNR();
                    freqError = radio.getFrequencyError();
                    Utils::println("<--- LoRa Packet Rx    : " + loraPacket);
                    Utils::println("(RSSI:" + String(rssi) + " / SNR:" + String(snr) + " / FreqErr:" + String(freqError) + ")");

                    if (true) { // Low power
                        ReceivedPacket receivedPacket;
                        receivedPacket.millis = millis();
                        receivedPacket.packet = loraPacket.substring(3);
                        receivedPacket.RSSI = rssi;
                        receivedPacket.SNR = snr;

                        if (receivedPackets.size() >= 20) {
                            receivedPackets.erase(receivedPackets.begin());
                        }

                        receivedPackets.push_back(receivedPacket);
                    }

                    return loraPacket;
                }                
            } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
                // timeout occurred while waiting for a packet
            } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
                Utils::println(F("CRC error!"));
                loraPacket = "";
            } else {
                Utils::print(F("failed, code "));
                Utils::println(String(state));
            }

            if (ignorePacket) {
                Utils::println("<--- LoRa Packet Rx    : " + loraPacket);
                Utils::println("Received own packet. Ignoring");

                ignorePacket = false;
                return "";
            }
        }
        return loraPacket;
    }

}