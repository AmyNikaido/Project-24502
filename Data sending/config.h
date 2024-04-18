#pragma once
#include <string>
using namespace std;

//  WiFi credentials
const char *SSID = "TP-Link_BE10";      // Wifiname
const char *PASSWORD = "SensorLab123$"; // Password

//  MQTT settings
const string ID = "ENGR498"; // Putt something different
const string BROKER = "broker.hivemq.com";
const string CLIENT_NAME = ID + "SensorHVAC";
const int TCP_PORT = 1883;

const string CLIENT_TELEMETRY_TOPIC = "HVACSenor";
