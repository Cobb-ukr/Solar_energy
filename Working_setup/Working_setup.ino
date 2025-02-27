#include <Wire.h>
#include <Adafruit_INA219.h>
#include <BH1750.h>
#include <SoftwareSerial.h>

#define LM35_PIN A2
#define VREF 5.00

Adafruit_INA219 ina219;
BH1750 lightMeter;
SoftwareSerial espSerial(2, 3);  // RX, TX (ESP8266 RX ← Arduino TX)

void setup() {
    Serial.begin(9600);       // Debugging on Serial Monitor
    
    Wire.begin();

    if (!ina219.begin()) {
        Serial.println("Failed to find INA219 chip");
        while (1) delay(10);
    }

    lightMeter.begin();
}

void loop() {
    float busVoltage = ina219.getBusVoltage_V();
    float shuntVoltage = ina219.getShuntVoltage_mV();
    float current_mA = ina219.getCurrent_mA();
    float power_mW = ina219.getPower_mW();
    float loadVoltage = busVoltage + (shuntVoltage / 1000);
    float lux = lightMeter.readLightLevel();
    float temperature = (analogRead(LM35_PIN) / 1023.0) * VREF * 100;

    // Format data as CSV
    String data = String(loadVoltage) + "," + String(current_mA) + "," +
                  String(power_mW) + "," + String(temperature) + "," + String(lux);

    Serial.println(data);  // Debugging output
    

    delay(3000);
}
