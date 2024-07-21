#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#define CONFIG_APRS_CALLSIGN "N0CALL-11" // APRS CALLSIGN with SSID

#define CONFIG_APRS_INTERVAL 1 // in minutes
#define CONFIG_APRS_OVERLAY "/"
#define CONFIG_APRS_SYMBOL "O"
#define CONFIG_APRS_PATH "WIDE1*" // we prefer WIDE1*
#define CONFIG_APRS_FLIGHT_ID 23  // flight number, used as N in comment

#define CONFIG_APRS_DIGI_MODE 2 // 0 for no Digi, 2 for SKY1-1 Digi

#define CONFIG_LORA_RX_FREQ 436.05
#define CONFIG_LORA_RX_SPEED "1200" // 1200 or 300 bps

#define CONFIG_LORA_TX_ACTIVE // comment to disable LoRa RX
#define CONFIG_LORA_RX_ACTIVE // comment to disable LoRa RX

// #define CONFIG_WSPR_ENABLE // comment to disable WSPR

#define CONFIG_WSPR_CALLSIGN "N0CALL" // WSPR CALLSIGN without SUFFIX/PREFIX
#define CONFIG_WSPR_SSID 1            // WSPR SSID will be used as SUFFIX like N0CALL/1

// #define CONFIG_DEBUG_ENABLE // comment to disable
// #define CONFIG_GPS_FORWARD_TO_SERIAL // comment to disable

// #define CONFIG_HISTORICAL_LOCATION_ENABLE // comment to disable historical location for LoRa APRS
// #define CONFIG_CURRENT_DAY_ENABLE // comment to disable current day

#endif