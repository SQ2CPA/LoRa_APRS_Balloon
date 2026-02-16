#ifndef PINS_CONFIG_H_
#define PINS_CONFIG_H_

#include <Arduino.h>

#ifdef HELTEC_HTCT62
#define RADIO_SCLK_PIN 10
#define RADIO_MISO_PIN 6
#define RADIO_MOSI_PIN 7
#define RADIO_CS_PIN 8
#define RADIO_RST_PIN 5
#define RADIO_DIO1_PIN 3
#define RADIO_BUSY_PIN 4
#endif

#endif