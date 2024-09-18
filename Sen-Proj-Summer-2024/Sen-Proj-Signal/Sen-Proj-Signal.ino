#include <WiFiNINA.h>
#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include "Sen-Proj-Signal-Secret.h"
#include "SDWiFi.h"

#define resistorValue = 1500;

// Wifi components (comment out all wifi stuff if not using)
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

// Dev board pins
const uint8_t analogPin = 16; // A1
const uint8_t OutputPin = 17; // A2
const uint8_t WiFiPin = 18; //A3
const uint8_t BattPin = 19; //A4

// ADC Read setup (Effects voltage division)
// voltageDiv = 2^(Resolution)
uint8_t readResolution = 8;

// Sample values
uint8_t i = 0;
uint8_t sampleVal = 0;
uint8_t samplePeriod = 1; //microseconds (1us)
uint16_t sampleAmt = 499;
uint16_t sampleArray[499]; // Array that will contain sample data
uint32_t timeArray[499];
uint32_t timeStart, timeFinish, timeTotal;
uint8_t sampleMinutes = 1;

// String for data
String dataString = "";

// String for Time stamp
String timeStamp = "";

//=============== TEST VALUE =================
uint8_t testVal = 1;
//============================================

// Pin to select SD chip (4)
const uint8_t chipSelect = 4;

// Data File component to save to SD
File dataFile;
char fileName[12] = {0};

// Data test
bool dataTest = true;

// Date and Time values
RTCZero rtc;
uint32_t epoch;
uint16_t s, m, h, d, mo, y;


  void setup() {
    // Increases sample rate
    ADC->CTRLA.bit.ENABLE = 0;                       // Disable the ADC
    while(ADC->STATUS.bit.SYNCBUSY);                 // Wait for synchronization
    ADC->SAMPCTRL.reg = 0x00;                        // Reduce the sample time by (63 + 1) * (48MHz / 512) / 2
    ADC->CTRLA.bit.ENABLE = 1;                       // Enable the ADC
    while(ADC->STATUS.bit.SYNCBUSY);                 // Wait for synchronization
    
    // put your setup code here, to run once:
    analogReference(AR_INTERNAL);
    analogReadResolution(readResolution);
    pinMode(OutputPin,OUTPUT);
    Serial.begin(9600);
    //while (!Serial);
    wifiConnect();
    initializeSDCard();
    timeSetup();
  }

  
  void loop() {
    getDateTime();
    sprintf(fileName, "%02d-%02d-%02d.csv", mo, d, y);
    
    dataFile = SD.open(fileName, FILE_WRITE);
    
    while(testVal != 0){
      getDateTime();
      sampleData();
      //dataToString();
      dataToSD();
      Serial.println(testVal);
      delay(5000);
      testVal--;
    }
      
    //sampleInterval();
    Serial.println("Finished Logging Data");
    exit(0);
  }

// Sets delay in minutes before the next round of sampling
  void sampleInterval(){
    for(uint16_t i = 0; i < sampleMinutes; i++){
      delay(1000);
      //delay(60000);
    }
  }

  void wifiConnect(){
    while(status != WL_CONNECTED){
      Serial.print("Attempting to connect to network: ");
      Serial.println(ssid);
      status = WiFi.begin(ssid, pass);
      // Wait 10 seconds for WiFi to connect (10,000 ms)
      delay(5000);
    }
    Serial.println("You're connected to the network");
    Serial.println("-------------------------------");
  }

  void initializeSDCard(){
    Serial.print("Initializing SD card...");

    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");
      // don't do anything more:
      while (1);
    }
    Serial.println("card initialized.");
  }


  void sampleData(){
    digitalWrite(OutputPin,HIGH);
    delay(1000);
    digitalWrite(OutputPin,LOW);
    timeStart = micros();
      for(uint16_t i = 0; i < sampleAmt; i++){
        timeFinish = micros();
        timeTotal = timeFinish - timeStart;
        timeArray[i] = timeTotal;
        sampleArray[i] = analogRead(analogPin);
      }
    Serial.println("data sampled.");
  }

  void dataToSD(){
    delay(10);
    //dataFile.println(timeStamp);
    dataFile.println(timeStart);
    for(uint16_t i = 0; i < sampleAmt; i++){
      float voltage = sampleArray[i] * (3.3/255); //========== DON'T FORGET TO SET FOR CORRECT RESOLUTION ==========
      dataString += timeArray[i];
      dataString += ";";
      dataString += String(voltage);
      dataString += "\n";
      dataFile.print(dataString);
      dataString = "";
    }
    dataFile.println(timeFinish);

    //dataFile.print(dataString);
    if(testVal == 0){
      dataFile.close();
      Serial.println("data file closed");
    }
    Serial.println("data logged.");
    dataFile.flush();
  }

    // Puts data into string (Sample Number; Voltage) for sending out via wifi or to SD card
  void dataToString(){
    if(dataString.length() != 0){
      dataString = "";
    }
    for(uint16_t j = 0; j < sampleAmt; j++){
        //unsigned long then = micros();
        unsigned int time = millis();
        ;
        timeArray[j] = time;
        float voltage = sampleArray[j] * (3.3/256);
        
        //unsigned long now = micros();
        //unsigned long time = now - then;

        //dataString += String(j);
        //dataString += ";";
        dataString += time;
        dataString += ";";
        dataString += String(voltage);
        dataString += "\n";
    }

    Serial.println("data to string");
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
    timeStamp = "";
    timeStamp += h;
    timeStamp += ":";
    timeStamp += m;
    timeStamp += ":";
    timeStamp += s;
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

  
