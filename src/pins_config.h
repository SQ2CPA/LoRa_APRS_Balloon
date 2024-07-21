#ifndef PINS_CONFIG_H_
#define PINS_CONFIG_H_

#include <Arduino.h>

#ifdef HELTEC_HTCT62
#define RADIO_SCLK_PIN 10 // SX1262 SCK
#define RADIO_MISO_PIN 6  // SX1262 MISO
#define RADIO_MOSI_PIN 7  // SX1262 MOSI
#define RADIO_CS_PIN 8    // SX1262 NSS
#define RADIO_RST_PIN 5   // SX1262 RST
#define RADIO_DIO1_PIN 3  // SX1262 DIO1
#define RADIO_BUSY_PIN 4  // SX1262 BUSY
#endif

#endif