# SQ2CPA LoRa APRS Balloon (+WSPR)

## Check my parts list [here](https://github.com/SQ2CPA/parts)

This firmware is suitable for LoRa based balloon trackers. For now its good for **HT-CT62** board that includes **ESP32C3+SX1262**. It contains many good features like historical location, current flight day detection and even digipeater so you can repeat your messages via balloon via LoRa APRS!

### Configuration

All configuration is located in `./src/configuration.h`.
You don't need to `Upload Filesystem Image`, all configuration is filled within code, not SPIFFS anymore!

### Pinout

-   `GPIO9` to VCC during flight, GND for programming.
-   GPS TX at `GPIO0`, GPS RX at `GPIO1` (I prefer ATGM336H-5N31)
-   SI5351 at SCL `GPIO19` and SDA `GPIO18` (for WSPR)

Based on **Ricardo CA2RXU** LoRa software that is available [here](https://github.com/richonguzman/LoRa_APRS_iGate).

# 73, Damian SQ2CPA, Poland
