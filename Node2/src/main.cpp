#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <esp_now.h>
#include <PubSubClient.h>

typedef struct sensor_data
{
    float temperature;
    uint8_t distance;
} sensor_data_t;

WiFiManager wifiManager;
WiFiClient wifiClient;

const uint8_t mqttServer[] = {0, 0, 0, 0};
const char *mqqtId = "85c92730-edda-11ee-bb4b-89d05c2d6ab2";
const char *mqttUser = "student@toi.toi";
const char *mqttPassword = "";
const uint16_t mqttPort = 3001;

PubSubClient mqttClient(mqttServer, mqttPort, wifiClient);

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
    if (len != sizeof(sensor_data_t))
    {
        Serial.println("Invalid packet received");
        return;
    }

    sensor_data_t sensorData;
    memcpy(&sensorData, incomingData, len);
    Serial.println("Distance: " + String(sensorData.distance) + " cm");
    Serial.println("Temperature: " + String(sensorData.temperature) + " °C");

    mqttClient.publish("v1/devices/me/telemetry", ("{temperature:" + String(sensorData.temperature) + ", distance:" + String(sensorData.distance) + "}").c_str());
}

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);

    wifiManager.setClass("invert");
    bool reboot = false;
    wifiManager.setAPCallback([&reboot](WiFiManager *mgr)
                              { reboot = true; });
    wifiManager.autoConnect("ESPAP-xzmitk01");
    if (reboot)
    {
        ESP.restart();
    }

    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        ESP.restart();
    }

    if(!mqttClient.connect(mqqtId, mqttUser, mqttPassword))
    {
        Serial.println("Failed to connect to MQTT server");
        ESP.restart();
    }

    esp_now_register_recv_cb(OnDataRecv);
}

void loop()
{
}
