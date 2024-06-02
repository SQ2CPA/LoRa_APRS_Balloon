#include "battery_utils.h"
#include "configuration.h"
#include "pins_config.h"

extern Configuration    Config;
extern uint32_t         lastBatteryCheck;

float adcReadingTransformation = (3.3/4095);
float voltageDividerCorrection = 0;

// for External Voltage Measurment (MAX = 15Volts !!!)
float R1 = 100.000; //in Kilo-Ohms
float R2 = 100.000; //in Kilo-Ohms
float readingCorrection = 0;
float multiplyCorrection = 0;


namespace BATTERY_Utils {

    float mapVoltage(float voltage, float in_min, float in_max, float out_min, float out_max) {
        return (voltage - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    float checkBattery() { 
        int sample;
        int sampleSum = 0;
        for (int i = 0; i < 100; i++) {
            #if defined(BATTERY_PIN)
            sample = analogRead(BATTERY_PIN);
            #else
            sample = 0;
            #endif
            sampleSum += sample;
            delayMicroseconds(50); 
        }

        float voltage = (2 * (sampleSum/100) * adcReadingTransformation) + voltageDividerCorrection;

        // return voltage; // raw voltage without mapping

        return mapVoltage(voltage, 3.32, 4.46, 3.2, 4.2); // mapped voltage
    }

    float checkExternalVoltage() {
        int sample;
        int sampleSum = 0;
        for (int i = 0; i < 100; i++) {
            sample = analogRead(Config.externalVoltagePin);
            sampleSum += sample;
            delayMicroseconds(50); 
        }

        float voltage = ((((sampleSum/100)* adcReadingTransformation) + readingCorrection) * ((R1+R2)/R2)) - multiplyCorrection;

        // return voltage; // raw voltage without mapping

        return mapVoltage(voltage, 5.24, 6.37, 4.5, 5.5); // mapped voltage
    }

}