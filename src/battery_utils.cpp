#include "battery_utils.h"
#include "configuration.h"
#include "pins_config.h"
//#include "lora_utils.h"

// Uncomment if you want to monitor voltage and sleep if voltage is too low (<3.15V)
//#define LOW_VOLTAGE_CUTOFF

float cutOffVoltage = 3.15;

extern Configuration    Config;
extern uint32_t         lastBatteryCheck;

float adcReadingTransformation = (3.3/4095);
float voltageDividerCorrection = 0.288;

// for External Voltage Measurment (MAX = 15Volts !!!)
float R1 = 100.000; //in Kilo-Ohms
float R2 = 27.000; //in Kilo-Ohms
float readingCorrection = 0.125;
float multiplyCorrection = 0.035;


namespace BATTERY_Utils {

    float mapVoltage(float voltage, float in_min, float in_max, float out_min, float out_max) {
        return (voltage - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
    }

    float checkBattery() { 
        int sample;
        int sampleSum = 0;
        for (int i = 0; i < 100; i++) {
            #if defined(TTGO_T_LORA32_V2_1) || defined(HELTEC_V2) || defined(HELTEC_WSL)
            sample = analogRead(batteryPin);
            #endif
            #if defined(HELTEC_V3) || defined(ESP32_DIY_LoRa) || defined(ESP32_DIY_1W_LoRa)
            sample = 0;
            #endif
            sampleSum += sample;
            delayMicroseconds(50); 
        }

        float voltage = (2 * (sampleSum/100) * adcReadingTransformation) + voltageDividerCorrection;

        return voltage; // raw voltage without mapping

        // return mapVoltage(voltage, 3.34, 4.71, 3.0, 4.2); // mapped voltage
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

        return voltage; // raw voltage without mapping

        // return mapVoltage(voltage, 5.05, 6.32, 4.5, 5.5); // mapped voltage
    }

    void checkIfShouldSleep() {
        #ifdef LOW_VOLTAGE_CUTOFF
        if (lastBatteryCheck == 0 || millis() - lastBatteryCheck >= 15 * 60 * 1000) {
            lastBatteryCheck = millis();

            float voltage = checkBattery();
            
            if (voltage < cutOffVoltage) {
                /*  Send message over APRS-IS or over LoRa???
                
                APRS_IS_Utils::upload("battery is low = sleeping")

                LoRa_Utils::sendNewMessage("battery is low = sleeping");
                
                */
                ESP.deepSleep(1800000000); // 30 min sleep (60s = 60e6)
            }
        }
        #endif
    }

}