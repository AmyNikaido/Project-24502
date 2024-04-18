#include <Arduino.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>

#include "config.h"

void connectWiFi()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi..");
    WiFi.begin(SSID, PASSWORD);
    delay(500);
  }

  Serial.println("Connected!");
}

WiFiClient wifiClient;
PubSubClient client(wifiClient);

void reconnectMQTTClient()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    if (client.connect(CLIENT_NAME.c_str()))
    {
      Serial.println("connected");
      // client.subscribe(CLIENT_TELEMETRY_TOPIC.c_str());
    }
    else
    {
      Serial.print("Retying in 5 seconds - failed, rc=");
      Serial.println(client.state());
      delay(5000);
    }
  }
}

void createMQTTClient()
{
  client.setServer(BROKER.c_str(), TCP_PORT);
  // client.setCallback(clientCallback);
  reconnectMQTTClient();
}

void setup()
{

  Serial.begin(9600);
  while (!Serial)
  {
    ;
  }

  connectWiFi();
  createMQTTClient();
}

void loop()
{
  reconnectMQTTClient();
  client.loop();

  string telemetry = "Sensor";
  client.publish(CLIENT_TELEMETRY_TOPIC.c_str(), telemetry.c_str());
  // put your main code here, to run repeatedly:
  // Serial.println("Hello");
  delay(2000);
}
