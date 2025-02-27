#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

const char* ssid = "91";
const char* password = "temp_pwd13";
const char* powerBiUrl = "https://api.powerbi.com/beta/191dd60d-090a-4459-a071-b03c80f3ac3b/datasets/6cbae5d0-f351-4734-85cf-adc2e51cd738/rows?experience=power-bi&key=qafvoL8E4Q91Xml5OLB6s1iruH39VaW9XtX6AUeUr5g0VlRurbSpBrbbNHbcfCgLh%2B9HR%2B9iT8DLilXdJpTEEg%3D%3D";  
const char* host = "api.powerbi.com"; 

WiFiClientSecure client;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000);

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");

    timeClient.begin();
    client.setInsecure(); // Bypass SSL verification
}

void loop() {
    timeClient.update();

    if (Serial.available()) {
        String sensorData = Serial.readStringUntil('\n');
        sensorData.trim();

        Serial.print("Received Sensor Data: ");
        Serial.println(sensorData);

        int commaCount = 0;
        for (int i = 0; i < sensorData.length(); i++) {
            if (sensorData[i] == ',') commaCount++;
        }

        if (commaCount == 4) {
            sendToPowerBI(sensorData);
        } else {
            Serial.println("Invalid sensor data format! Skipping...");
        }
    }
}

void sendToPowerBI(String data) {
    Serial.println("Connecting to Power BI...");

    if (!client.connect(host, 443)) {
        Serial.println("Failed to connect to Power BI! Retrying in 5 seconds...");
        delay(5000);
        return;
    }

    int firstComma = data.indexOf(',');
    int secondComma = data.indexOf(',', firstComma + 1);
    int thirdComma = data.indexOf(',', secondComma + 1);
    int fourthComma = data.indexOf(',', thirdComma + 1);

    String voltage = data.substring(0, firstComma);
    String current = data.substring(firstComma + 1, secondComma);
    String power = data.substring(secondComma + 1, thirdComma);
    String temperature = data.substring(thirdComma + 1, fourthComma);
    String lux = data.substring(fourthComma + 1);

    String timestamp = timeClient.getFormattedTime();

    String jsonPayload = "[{\"Timestamp\": \"" + timestamp + "\", \"Voltage\": " + voltage +
                         ", \"Current\": " + current + ", \"Power\": " + power +
                         ", \"Temperature\": " + temperature + ", \"Light\": " + lux + "}]";

    Serial.println("Sending JSON Data: " + jsonPayload);

    client.print("POST " + String(powerBiUrl) + " HTTP/1.1\r\n");
    client.print("Host: " + String(host) + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: " + String(jsonPayload.length()) + "\r\n\r\n");
    client.print(jsonPayload);

    Serial.println("Data sent! Waiting for response...");

    while (client.available()) {
        String response = client.readStringUntil('\r');
        Serial.println("Response: " + response);
    }

    client.stop();
}
