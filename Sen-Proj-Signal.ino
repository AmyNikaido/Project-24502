#include <WiFiNINA.h>
#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>
#include "Sen-Proj-Signal-Secret.h"

#define resistorValue = 1000;

// Wifi components (comment out all wifi stuff if not using)
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

// Dev board pins
int OutputPin = 20; //A5
int analogPin = 16; //A1

// ADC Read setup
int readResolution = 12;

// Sample values
int i = 0;
int sampleVal = 0;
unsigned int samplePeriod = 1; //microseconds (1ms)
unsigned int sampleAmt = 499;
unsigned int sampleArray[499]; // Array that will contain sample data
int sampleMinutes = 1;

// String for data
String dataString = "";

// Pin to select chip (4)
const int chipSelect = 4;

// Data File component to save to SD
File dataFile;

// Date and Time values
RTCZero rtc;
unsigned long epoch;
int s, m, h, d, mo, y;

  void setup() {
    // put your setup code here, to run once:
    analogReference(AR_INTERNAL);
    analogReadResolution(readResolution);
    pinMode(OutputPin,OUTPUT);
    //Serial.begin(115200);
    //while (!Serial);

    while(status != WL_CONNECTED) {
      Serial.print("Attempting to connect to network: ");
      Serial.print(ssid);

      status = WiFi.begin(ssid, pass);

      // Wait 10 seconds for WiFi to connect (10,000 ms)
      delay(10000);
    }

    Serial.println("You're connected to the network");
    Serial.println("-------------------------------");

    Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      // don't do anything more:
      while (1);
    }
    Serial.println("card initialized.");

    timeSetup();
  }


  
  void loop() {
    getDateTime();
    sampleData();
    dataToString();
    dataToSD();
    //printToSerial(); // Essentially where to grab sampled data
    //sampleInterval();
    exit(0);
  }

  
  void sampleData(){
    digitalWrite(OutputPin,HIGH);
    delay(1000);
    digitalWrite(OutputPin,LOW);
      for(int i = 0; i < sampleAmt; i++){
        sampleArray[i] = analogRead(analogPin);
      }
    
  }

    // Sets delay in minutes before the next round of sampling
  void sampleInterval(){
    for(int i = 0; i < sampleMinutes; i++){
      delay(60000);
    }
  }

  // Puts data into string (Sample Number; Voltage) for sending out via wifi or to SD card
  void dataToString(){
    for(int j = 0; j < sampleAmt; j++){
        float voltage = sampleArray[j] * (3.3/4096);
        dataString += String(j);
        dataString += ";";
        dataString += String(voltage);
        dataString += "\n";
    }

    //Serial.println(dataString);
  }

  void dataToSD(){
    dataFile = SD.open("datalog.txt", FILE_WRITE);
    dataFile.print(dataString);
    dataFile.close();
    Serial.println("data logged.");
  }

  // Gets UTC time WRONG DAY. Can figure out later or get correct from GUI/API
  void timeSetup(){
    rtc.begin();
    epoch = WiFi.getTime();
    rtc.setEpoch(epoch);
  }

  // For AZ Timezone from UTC
  void getDateTime(){
    y = rtc.getYear();
    mo = rtc.getMonth();
    d = rtc.getDay();
    h = rtc.getHours();
    m = rtc.getMinutes();
    s = rtc.getSeconds();
  }

  // For testing to make sure data looks correct
  void printToSerial(){
    for(int j = 0; j < sampleAmt; j++){
        float voltage = sampleArray[j] * (3.3/4096);
        Serial.println(voltage);
    }

    Serial.print("Time: ");
    Serial.print(h);
    Serial.print(":");
    Serial.print(m);
    Serial.print(":");
    Serial.print(s);

    Serial.print(" Date: ");
    Serial.print(d);
    Serial.print("/");
    Serial.print(mo);
    Serial.print("/");
    Serial.print(y);
  }

  
