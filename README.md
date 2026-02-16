# SQ2CPA LoRa APRS Balloon (+WSPR!)

## Check my parts list [here](https://github.com/SQ2CPA/parts)

## This software is experimental!

We are playing with Balloon as LoRa APRS Digipeater testing LoRa modulation parameters and other stuff!

## Description

This firmware is suitable for LoRa based balloon trackers. For now its good for **HT-CT62** board that includes **ESP32C3+SX1262**. It contains many good features like digipeater so you can repeat your messages via balloon via LoRa APRS!

You can also easily modify that for other LoRa modules like SX1278 or other ESP32. This is not hard!

### Configuration

All configuration is located in `./src/configuration.h`.
You don't need to `Upload Filesystem Image`, all configuration is filled within code, not SPIFFS anymore!

### Pinout

- `GPIO9` to VCC during flight, GND for programming.
- GPS TX at `GPIO0`, GPS RX at `GPIO1` (I prefer ATGM336H-5N31)
- SI5351 at SCL `GPIO19` and SDA `GPIO18` (only for WSPR!)

### Tips

- Use external BOD 2.8-3V for ESP32 because of broken Internal BOD
- `GPIO9` to GND for programming, to VCC during flight (more safety)
- Use 3.3V LDO if voltage >3.5V due to HT-CT62 datasheet (more safety, I don't know what is the real good max. voltage)
- You don't need external I2C pull-up resistors for WSPR

Based on **Ricardo CA2RXU** LoRa software that is available [here](https://github.com/richonguzman/LoRa_APRS_iGate).

# 73, Damian SQ2CPA, Poland
