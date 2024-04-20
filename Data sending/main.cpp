#include <WiFiNINA.h>
#include <RTCZero.h>
#include <SPI.h>

#include <Arduino.h>
#include <PubSubClient.h>

#include "config.h"

// Function prototypes
void sampleData();
void printToSerial();
void dataToString();

String dataString = "";
int readResolution = 12;
int analogPin = A1; // Use A1 for analog pin
int OutputPin = A5; // Use A5 for digital output pin
unsigned int sampleAmt = 499;
unsigned int sampleArray[499];

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
      client.subscribe(CLIENT_TELEMETRY_TOPIC.c_str());
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

  analogReference(AR_INTERNAL);
  analogReadResolution(readResolution);
  pinMode(OutputPin, OUTPUT);
  Serial.begin(9600);
  while (!Serial)
    ;
}

void loop()
{

  reconnectMQTTClient();
  client.loop();

  sampleData();
  dataToString();
  printToSerial();

  for (int j = 0; j < sampleAmt; j++)
  {
    float voltage = sampleArray[j] * (3.3 / 4096);
    String voltageStr = String(voltage);
    client.publish(CLIENT_TELEMETRY_TOPIC.c_str(), voltageStr.c_str());
  }

  // string telemetry = "Sensor";
  // client.publish(CLIENT_TELEMETRY_TOPIC.c_str(), telemetry.c_str());
  //  put your main code here, to run repeatedly:
  //  Serial.println("Hello");
  // delay(2000);

  exit(0); // Exiting loop, assuming you want to do this for testing purposes
}

void sampleData()
{
  digitalWrite(OutputPin, HIGH);
  delay(1000);
  digitalWrite(OutputPin, LOW);
  for (int i = 0; i < sampleAmt; i++)
  {
    sampleArray[i] = analogRead(analogPin);
  }
}

void printToSerial()
{
  for (int j = 0; j < sampleAmt; j++)
  {
    float voltage = sampleArray[j] * (3.3 / 4096);
    Serial.println(voltage);
  }
}

void dataToString()
{
  for (int j = 0; j < sampleAmt; j++)
  {
    float voltage = sampleArray[j] * (3.3 / 4096);
    dataString += String(j) + ";" + String(voltage) + "\n";
  }
}
