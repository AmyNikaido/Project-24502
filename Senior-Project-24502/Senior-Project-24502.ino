/*

Team 24502: Wireless Sample Mode Response Technique Sensor for Power Supplies and HVAC Systems

*/
#include <WiFiNINA.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include <TimeLib.h>
#include <Wire.h>
#include "arduino_secrets.h"

//This is used to determine when the Arduino has switched to using battery power.
#define PMIC_ADDRESS 0x6B
// this is for measuring battery voltage.
const long InternalReferenceVoltage = 1062;  // Adjust this value to your board's specific internal BG voltage
bool LiPoFlag = false; //If false, Arduino is not using LiPo power. Else, battery is in use. 

// constants won't change. They're used here to set pin numbers:
const int ledPin = LED_BUILTIN;    // the number of the LED pin
const int RELAY_PIN1 = 3;  // the Arduino pin, which connects to the IN pin of the second pair of relays. This pair of relays cuts off the capacitor/resistor combo from the power source.
const int RELAY_PIN2 = 4;  // the Arduino pin, which connects to the IN pin of the first pair of relays. This pair of relays controls the sensor input/sensing.
const int analogPin = 16; // A1, the Arduino pin that will read voltage
const int pulsePin = 20; //A5, the Arduino pin that will pulse in a voltage to the capacitor
const int chipSelect = 4; //Pin that will be used for the SD card
const long bleedInterval = 20000;  //Interval or time it takes for the capacitor to bleed out to near 0V. This number is obtained from the capacitor bleed equation.

// Variables will change:
bool relayPair_1 = false; //This bool variable controls when the resistor and capacitor are cut off from the main circuit.
bool relayPair_2 = false; //This pair of relays controls the sensor input/sensing.
bool readingActive = false; //This bool variable determines if the sensing process is active or not.
char command; //Using serial commands for testing purposes
int status = WL_IDLE_STATUS; //WiFi status

//Network info
char ssid[] = SECRET_SSID;    // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long previousMillis = 0;
unsigned long currentMillis = 0; 

// Sample values
uint8_t i = 0;
uint8_t sampleVal = 0;
uint8_t samplePeriod = 1; //microseconds (1us)
uint16_t sampleAmt = 500;
uint16_t sampleArray[500]; // Array that will contain sample data
uint16_t sampleArrayInverse[500]; //Array that contains the inverse of the sampla data
uint32_t timeArray[500]; //Array that will contain the time of the sample data
uint32_t timeStart, timeFinish, timeTotal, dataTime, epochTime;
uint8_t sampleMinutes = 1;

// String for data
String dataString = "";

// Data File component to save to SD
File dataFile;
char fileName[13];

// Date and Time values
uint32_t epoch;

//ts_ stands for time stamp. These are
uint16_t ts_year;
uint16_t ts_month;
uint16_t ts_day;
uint16_t ts_hour;
uint16_t ts_minute;
uint16_t ts_second;

void setup() {

  Serial.begin(9600);
  Wire.begin();
  
  initializeSD(); //Initializes the SD card module
  initializeWiFi();

  pinMode(ledPin, OUTPUT);
  pinMode(pulsePin, OUTPUT);
  pinMode(RELAY_PIN1, OUTPUT);
  pinMode(RELAY_PIN2, OUTPUT);

  // set initial LED state
  digitalWrite(ledPin, LOW);

  // set initial relay states
  digitalWrite(RELAY_PIN1, HIGH); //If HIGH, circuit is open and current/voltage doesn't passed through, if LOW, circuit closes and current/voltage passes through
  digitalWrite(RELAY_PIN2, HIGH); //If HIGH, circuit is closed and allows current/voltage through, If LOW, circuit is opened

}

void loop() { 
  
  PowerStatus();

  recvCMD();

  if (readingActive == true){
    relaySet1();
    relaySet2();
    sampleData();
    sampleDataToString();
  }

  else if (readingActive == false){
    relaySet2();
    delay(1000);
    relaySet1();
  }
}

void recvCMD() { //This function determines what kind of command was sent to the serial comm. Will be replaced with WiFi commands later.

  if (Serial.available() > 0) {
    command = Serial.read();
    Serial.println(command);

      if (command == 'A'){
      readingActive = true;
      Serial.println(readingActive);
      Serial.println("Sensor and relays activated");
    }
      if (command == 'B'){ //Used for testing purposes
      Serial.println(dataString);
      getTime();
    }
      if (command == 'C'){
      Serial.println("Saving data to SD card...");
      saveDataSD();
    }
  }
}

void relaySet1(){ //This function activates the first pair of relays that will cut off the capacitor and resistor combo from the main power source.
  
  if (readingActive == true){
    digitalWrite(ledPin, HIGH);
    digitalWrite(RELAY_PIN1, LOW); //If HIGH, circuit is closed and allows current/voltage through, If LOW, circuit is opened

    delay(bleedInterval);
  }

  else if(readingActive == false){
    digitalWrite(ledPin, LOW);
    digitalWrite(RELAY_PIN1, HIGH); //If HIGH, circuit is closed and allows current/voltage through, If LOW, circuit is opened
  }

}

void relaySet2(){ //This function activates the second pair of relays that will connect the arduino to the resistor and capacitor, and allow it to pulse a voltage and take readings

  if (readingActive == true){
    digitalWrite(RELAY_PIN2, LOW); 
  }

  else if(readingActive == false){
    digitalWrite(RELAY_PIN2, HIGH); 
  }
}

void sampleData(){ //This function pulses in 3.3 volts into the capactitor then samples the decaying voltage into an array, takes readings from the analog pin, and saves that data into an array

  digitalWrite(pulsePin,HIGH);
  delay(1000);
  digitalWrite(pulsePin,LOW);

  timeStart = micros();
    for(int i = 0; i < sampleAmt; i++){ //This takes the readings from the analog pin and puts it in an array

        timeFinish = micros();
        timeTotal = timeFinish - timeStart;
        timeArray[i] = timeTotal;
        sampleArray[i] = analogRead(analogPin);
    }

  Serial.println("data sampled.");
  readingActive = false;
}

//NEED TO ADD INVERSE DATA
void sampleDataToString(){ //This function parses the sample and time arrays and formats them into a string. 

  if(dataString.length() != 0){
    dataString = "";
  }

  for(int j = 0; j < sampleAmt; j++){
     
      float voltage = sampleArray[j] * (3.3 / 1023.0);
      float voltageInverse = 1/voltage;
      dataTime = timeArray[j];

      dataString += dataTime;
      dataString += ",";
      dataString += voltage;
      dataString += ",";
      dataString += voltageInverse;
      dataString += "\n";
    }

  Serial.println("data to string");
}

void initializeSD(){ //This function initializes the SD card. If the SD card is not present, the Arduino will not function

  if (!SD.begin(chipSelect)) {// If an SD card is not present, do not initialize
    // don't do anything more:
    while (1);
  }

  Serial.println("SD card initialized");

}

void initializeWiFi(){ //This function initializes the WiFi module and connects to the network defined in arduino_secrets.h

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  /*
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  */
  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.println("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");

}

void saveDataSD(){ //This array takes the data string and saves it to the SD card as a .CSV file

  getTime();
  SdFile::dateTimeCallback(DateTimeSD);

  sprintf(fileName, "%02d%02d%02d%02d.csv",ts_month, ts_day, ts_hour, ts_minute);
  dataFile = SD.open(fileName, FILE_WRITE);

  dataFile.println(dataString);
  dataFile.close();
  dataFile.flush();
  Serial.println("data saved.");
}

void getTime(){ //This function takes the epoch time from WiFi and then grabs the date and time from that

  epochTime = WiFi.getTime();

  ts_year = year(epochTime);
  ts_month = month(epochTime);
  ts_day = day(epochTime);
  ts_hour = hour(epochTime);
  ts_minute = minute(epochTime);
  ts_second = second(epochTime);

  sprintf(fileName, "%02d%02d%02d%02d.csv",ts_month, ts_day, ts_hour, ts_minute);

  Serial.println(fileName);

}

void DateTimeSD (uint16_t* date, uint16_t* time){ //This function adds a timestamp to the file that Windows will display in the file information

  *date = FAT_DATE(ts_year, ts_month, ts_day);
  *time = FAT_TIME(ts_hour, ts_minute, ts_second);

}

//This function determines if the Arduino is running off of main power or the LiPo battery
//Code taken from https://forum.arduino.cc/t/mkr1010-how-to-detect-active-power-supply/1076615/3
void PowerStatus(){

  int sensorValue = analogRead(ADC_BATTERY);
  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 4.3V):
  float voltage = sensorValue * (4.3 / 1023.0);

  byte reg, val;
  byte BQ_REG[1] = { 0x08 }; // reading only REG08

  for (int r = 0; r < sizeof(BQ_REG); r++) {

    reg = BQ_REG[r];

    Wire.beginTransmission(PMIC_ADDRESS);
    Wire.write((byte)reg);
    Wire.endTransmission();
    
    Wire.beginTransmission(PMIC_ADDRESS);
    Wire.requestFrom(PMIC_ADDRESS, 1);
    Wire.endTransmission();
    
    val = Wire.read() & B00000100; // reg 8 bit 2 is the Power Good signal - USB 5V is connected
  }

  //Val == 4 implies Arduino is being powered by main power source. Any other value implies Arduino is being powered from LiPo battery.
  if (val == 4) {
      //if (debug) Serial.println("  USB 5V Power Supply");
      digitalWrite(ledPin, HIGH);  // turn the LED on if reg8 bit 2 is true
      LiPoFlag = false;
    } else {
      digitalWrite(ledPin, LOW);  
      LiPoFlag = true;
    }

}

void WiFiStatus(){ //This function attempts to reconnect the Arduino if the connection is dropped. NEEDS TESTING.

  status = WiFi.status();
  
  delay(100);

  if(status == WL_CONNECTED){
    //Do nothing, get out of function
  }

  else{

    while (status != WL_CONNECTED) {
    Serial.println("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
    }
  }

}





