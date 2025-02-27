#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "env.h"

// WiFi Credentials
const char WIFI_SSID[] = "";
const char WIFI_PASSWORD[] = "";

// AWS IoT Details
const char THINGNAME[] = "";
const char MQTT_HOST[] = "";
const char AWS_IOT_PUBLISH_TOPIC[] = "esp8266/pub";

// Timezone offset
const int8_t TIME_ZONE = -5;
const long interval = 5000; // Publish interval
unsigned long lastMillis = 0;

// WiFi & MQTT Client
WiFiClientSecure net;
BearSSL::X509List cert(cacert);
BearSSL::X509List client_crt(client_cert);
BearSSL::PrivateKey key(privkey);
PubSubClient client(net);

// Function to connect to NTP server
void NTPConnect() {
    Serial.print("Setting time using SNTP...");
    configTime(TIME_ZONE * 3600, 0, "pool.ntp.org", "time.nist.gov");
    time_t now = time(nullptr);
    while (now < 1510592825) {
        delay(500);
        Serial.print(".");
        now = time(nullptr);
    }
    Serial.println(" done!");
}

// Function to connect to AWS IoT Core
void connectAWS() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    digitalWrite(LED_BUILTIN, HIGH);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println(" Connected!");

    NTPConnect(); // Set system time

    net.setTrustAnchors(&cert);
    net.setClientRSACert(&client_crt, &key);
    client.setServer(MQTT_HOST, 8883);

    Serial.print("Connecting to AWS IoT...");
    while (!client.connect(THINGNAME)) {
        Serial.print(".");
        delay(1000);
    }
    
    Serial.println(" Connected to AWS IoT!");
    digitalWrite(LED_BUILTIN, LOW);

}

// Function to publish sensor data to AWS IoT
void publishMessage(float voltage, float current, float power, float temperature, float light) {
    StaticJsonDocument<200> doc;
    doc["device_id"] = THINGNAME;
    doc["voltage"] = voltage;
    doc["current"] = current;
    doc["power"] = power;
    doc["temperature"] = temperature;
    doc["light"] = light;

    char jsonBuffer[200];
    serializeJson(doc, jsonBuffer);
    client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
    Serial.println("Data Sent to AWS IoT: " + String(jsonBuffer));
}

void setup() {
    Serial.begin(9600); // For Arduino communication
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.println("ESP8266 Serial Ready");
    connectAWS();
}

void loop() {
    if (Serial.available()) {
        String data = Serial.readStringUntil('\n'); // Read Arduino output
        Serial.println("Received: " + data);

        // Parse comma-separated values
        float voltage, current, power, temperature, light;
        sscanf(data.c_str(), "%f,%f,%f,%f,%f", &voltage, &current, &power, &temperature, &light);

        // Publish to AWS IoT
        if (millis() - lastMillis > interval) {
            lastMillis = millis();
            if (client.connected()) {
                publishMessage(voltage, current, power, temperature, light);
            }
        }
    }

    client.loop(); // Maintain MQTT connection
}
