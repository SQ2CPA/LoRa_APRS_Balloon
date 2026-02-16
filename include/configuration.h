#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define CONFIG_APRS_CALLSIGN "N0CALL-11" // APRS CALLSIGN with SSID (-11 prefered for balloons)

// #define CONFIG_APRS_FLIGHT_ID 1 // flight number, used as N in comment

// #define CONFIG_APRS_DIGI_MODE // enable Digi (from RX frequency, path SKY1, SKY2 or SKY3)

#define CONFIG_LORA_RX_FREQ 436.05 // Only for RX!!

#define CONFIG_LORA_RX_ACTIVE // enable LoRa RX

#define CONFIG_LORA_MIN_POWER 15 // min LoRa power in dBm
#define CONFIG_LORA_MAX_POWER 20 // max LoRa power in dBm

// #define CONFIG_WSPR_ENABLE // enable WSPR

#define CONFIG_WSPR_CALLSIGN "N0CALL" // WSPR CALLSIGN without SUFFIX/PREFIX
#define CONFIG_WSPR_SUFFIX 0          // WSPR SSID will be used as SUFFIX like N0CALL/1

#define CONFIG_WSPR_STARTUP_TONE_ENABLE // enable startup tone at 144.025 MHz (only when using WSPR!)
#define CONFIG_LORA_STARTUP_TONE_ENABLE // enable startup tone at 433.025 MHz (from SX1262)

// #define CONFIG_DEBUG_ENABLE
// #define CONFIG_GPS_FORWARD_TO_SERIAL // enable gps to serial forward

#define CONFIG_GPS_PIN_TX 0 // GPS TX at GPIO0
#define CONFIG_GPS_PIN_RX 1 // GPS RX at GPIO1

#define CONFIG_GPS_PIN_EN 2 // GPS ON/OFF at GPIO2

// #define CONFIG_GPS_WATCHDOG // Enable GPS watchdog

#endif