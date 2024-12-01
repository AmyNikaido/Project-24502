/*

Team 24502: Wireless Sample Mode Response Technique Sensor for Power Supplies and HVAC Systems

*/
#include <WiFiNINA.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include <TimeLib.h>
#include <Wire.h>
#include <ArduinoLowPower.h>
#include <ArduinoHttpClient.h>
#include "arduino_secrets.h"

#define resistorValue 1500
#define sampleAmt 500 //Allows users to quickly change sample amount w/o scrolling through code
#define bleedInterval 20000 //Interval or time it takes for the capacitor to bleed out to near 0V. This number is obtained from the capacitor bleed equation.

//This may not work 100% but worth a try if we need to edit quickly
//#define voltDecayDiv 4096
//#define ringDiv 4.096

// Dev board pins
const uint8_t relayPin1 = 15; //A0 This pair of relays cuts off the capacitor/resistor combo from the power source.
const uint8_t relayPin2 = 16; //A1 This pair of relays controls the voltage decay input/sensing.
const uint8_t relayPin3 = 17; //A2 This pair of relays controls the RingDown input/sensing.
const uint8_t VoltDecayPulse = 18; //A3 Arduino pin that pulses in voltage for the RC circuit
const uint8_t VoltDecayRead = 19; //A4 Arduino pin that will read voltage from the RC circuit
const uint8_t RingDownRead = 20; // A5 the Arduino pin that will read voltage from the LC circuit
const uint8_t RingDownPulse = 21; // A6 Arduino pin that will pulse in a voltage to the LC Circuit
const int chipSelect = 4; //Pin that will be used for the SD card

// ADC Read setup (Effects voltage division)
// voltageDiv = 2^(Resolution)
uint8_t readResolution = 12;

// Variables that will change:
bool readingActive = false; //This bool variable determines if the sensing process is active or not. Can be sent out as a flag
bool LiPoFlag = false; //If false, Arduino is not using LiPo power. Else, battery is in use.
int status; 
char command; //Using serial commands for testing purposes

//Variables used to determine when automatic reading takes place.
unsigned long startMillis;  
unsigned long currentMillis;
//const unsigned long readPeriod = 3600000;  //1 Hour
const unsigned long readPeriod = 60000;  //1 minute

// String for data
String dataString = "";

//Wireless comm stuff
char serverAddress[] = "192.168.0.3";  // server address
char uploadEndpoint[] = "/upload";
IPAddress ip;
int port = 8080;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

// Data File component to save to SD card
File dataFile;
char fileName[13];

// Date and timestamp values
uint32_t epochTime;
uint16_t ts_year;
uint16_t ts_month;
uint16_t ts_day;
uint16_t ts_hour;
uint16_t ts_minute;
uint16_t ts_second;


void setup() {
  // This section sets up the ADC for a faster sample rate
  ADC->CTRLA.bit.ENABLE = 0;                    // Disable ADC
  while(ADC->STATUS.bit.SYNCBUSY == 1);         // Wait for synchronization
  ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV64;   // Divide Clock by 512.
  ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 | ADC_AVGCTRL_ADJRES(0x00ul);  // 1 sample | Adjusting result by 0
  ADC->SAMPCTRL.reg = 0x00;                     // Sampling Time Length = 0
  ADC->CTRLA.bit.ENABLE = 1;                    // Enable ADC
  analogReadResolution(readResolution);
  analogReference(AR_INTERNAL);
  Serial.begin(9600);

  Wire.begin();

  pinMode(relayPin1,OUTPUT);
  pinMode(relayPin2,OUTPUT);
  pinMode(relayPin3,OUTPUT);
  pinMode(VoltDecayPulse,OUTPUT);
  pinMode(VoltDecayRead,INPUT);
  pinMode(RingDownPulse,OUTPUT);
  pinMode(RingDownRead,INPUT);

  // set initial relay states
  digitalWrite(relayPin1, HIGH); //If HIGH, circuit is open and current/voltage doesn't passed through, if LOW, circuit closes and current/voltage passes through
  digitalWrite(relayPin2, HIGH); //If HIGH, circuit is closed and allows current/voltage through, If LOW, circuit is opened
  digitalWrite(relayPin3, HIGH); //If HIGH, circuit is closed and allows current/voltage through, If LOW, circuit is opened

  //initializeSD(); //Initializes the SD card module
  initializeWiFi(); //Initializes WiFi

  startMillis = millis();  //initial start time

}

void loop() { 
  
  currentMillis = millis();

  PowerSourceStatus(); //checks power status

  WiFiStatus(); //checks WiFi status
  

  while (!LiPoFlag){

    httpCommand(); //receives commands
    autoRead(); //Takes voltage decay and ringdown at specific intervals

  }
  
}

void relaySet1(){ //This function activates the first pair of relays that will cut off the capacitor and resistor combo from the main power source.
  
  if (readingActive == true){
    digitalWrite(relayPin1, LOW); //If HIGH, circuit is closed and allows current/voltage through, If LOW, circuit is opened

  }

  else if(readingActive == false){
    //digitalWrite(ledPin, LOW);
    digitalWrite(relayPin1, HIGH); //If HIGH, circuit is closed and allows current/voltage through, If LOW, circuit is opened
  }

}

void relaySet2(){ //This function activates the second pair of relays that will connect the arduino to the resistor and capacitor, and allow it to pulse a voltage and take readings

  if (readingActive == true){
    digitalWrite(relayPin2, LOW);
  }

  else if(readingActive == false){
    digitalWrite(relayPin2, HIGH); 
  }
}

void relaySet3(){ //This function activates the second pair of relays that will connect the arduino to the resistor and capacitor, and allow it to pulse a voltage and take readings

  if (readingActive == true){
    digitalWrite(relayPin3, LOW); 
  }

  else if(readingActive == false){
    digitalWrite(relayPin3, HIGH); 
  }
}

void rcReading(){ //This function is the RC decay functionality

  relaySet1();
  delay(bleedInterval); 
  relaySet2();
  sampleVoltageDataAndToString();
  //saveDataToSD();
  delay(1000);
  relaySet2();
  delay(1000);
  relaySet1();

}

void lcReading(){

  relaySet1();
  delay(bleedInterval); 
  relaySet2();
  relaySet3();
  sampleRingDownDataAndToString();
  //saveDataToSD();
  delay(1000);
  relaySet3();
  delay(1000);
  relaySet2();
  delay(1000);
  relaySet1();
}

void sampleVoltageDataAndToString(){ //This function pulses in 3.3 volts into the capacitor then samples the decaying voltage into an array, takes readings from the analog pin, and saves that data into an array

  // These can probably go up top before the set up so we're not creating ints when we want to sample, not sure if its for http transfer though
  uint32_t dataTime;
  uint32_t timeStart, timeFinish, timeTotal;
  uint32_t timeArray[sampleAmt]; //Array that will contain the time of the sample data  //if using the sampleAmt: uint32_t timeArray[sampleAmt];
  uint16_t sampleArray[sampleAmt]; // Array that will contain sample data  //if using the sampleAmt: uint16_t sampleArray[sampleAmt];

  delay(500);

  digitalWrite(VoltDecayPulse,HIGH);
  delay(2000);
  digitalWrite(VoltDecayPulse,LOW);

  timeStart = micros();

  for(int i = 0; i < sampleAmt; i++){ //This takes the readings from the analog pin and puts it in an array

    timeFinish = micros();
    timeTotal = timeFinish - timeStart;
    timeArray[i] = timeTotal;
    sampleArray[i] = analogRead(VoltDecayRead);

  }

  Serial.println("data sampled");


  if(dataString.length() != 0){
  
    dataString = "";

  }

  dataString += "Time";
  dataString += ",";
  dataString += "Voltage";
  dataString += "\n";

  for(int j = 0; j < sampleAmt; j++){

    Serial.println(j);

    //float voltage = sampleArray[j] * (3.3 / voltDecayDiv); // If using voltDecayDiv
    //float voltage = sampleArray[j] * (3.3 / 4096.0); // If voltDecayDiv doesn't work
    float voltage = sampleArray[j] * (3.3 / 1023.0); // this would need to go to 4096.0 for 12-bit samples
      
    dataTime = timeArray[j];

    dataString += dataTime;
    dataString += ",";
    dataString += voltage;
    dataString += "\n";

  }
    
  Serial.println("data to string");

  readingActive = false;
}

void sampleRingDownDataAndToString(){ //This takes readings for the Ringdown circuit and puts it into an array

  uint32_t dataTime;
  uint32_t timeStart, timeFinish, timeTotal;
  uint32_t timeArray[sampleAmt]; //Array that will contain the time of the sample data
  uint16_t sampleArray[sampleAmt]; // Array that will contain sample data
    
  delay(1000);  //Delay a second to make sure relays have switched and all is ready to sample

  timeStart = micros();

    for(uint16_t i = 0; i < sampleAmt; i++){
      digitalWrite(RingDownPulse,HIGH);  //Pin needs to stay high while sampling to get a good ring
      timeFinish = micros();
      timeTotal = timeFinish - timeStart;
      timeArray[i] = timeTotal;
      sampleArray[i] = analogRead(RingDownRead);
    }

  digitalWrite(RingDownPulse,LOW);
  Serial.println("data sampled.");

  if(dataString.length() != 0){
    dataString = "";
  }

  dataString += "Time";
  dataString += ",";
  dataString += "Voltage";
  dataString += "\n";

  for(int j = 0; j < sampleAmt; j++){

    Serial.println(j);
    //float voltage = sampleArray[j] * (3.3 / ringDiv); //If using ringDiv
    float voltage = sampleArray[j] * (3.3 / 4.096); // this needs to 4.096 so I can read at millivolts ~600mV
    dataTime = timeArray[j];

    dataString += dataTime;
    dataString += ",";
    dataString += voltage;
    dataString += "\n";
  }
  Serial.println("data to string");
  readingActive = false;
}

void saveDataToSD(){ //This function saves data onto a .csv file, and posts the data to the server.

  epochTime = WiFi.getTime();

  ts_year = year(epochTime);
  ts_month = month(epochTime);
  ts_day = day(epochTime);
  ts_hour = hour(epochTime);
  ts_minute = minute(epochTime);
  ts_second = second(epochTime);

  SdFile::dateTimeCallback(DateTimeSD);

  sprintf(fileName, "%02d%02d%02d%02d.csv",ts_month, ts_day, ts_hour, ts_minute);
  dataFile = SD.open(fileName, FILE_WRITE);

  dataFile.println(dataString);

  dataFile.close();
  dataFile.flush();
  Serial.println("data saved.");

}

/*
void initializeSD(){ //This function initializes the SD card. If the SD card is not present, the Arduino will not function

  if (!SD.begin(chipSelect)) {// If an SD card is not present, do not initialize
    // don't do anything more:
    while (1);
  }

  Serial.println("SD card initialized");

}*/

void DateTimeSD (uint16_t* date, uint16_t* time){ //This function adds a timestamp to the file that Windows will display in the file information

  *date = FAT_DATE(ts_year, ts_month, ts_day);
  *time = FAT_TIME(ts_hour, ts_minute, ts_second);

}

void initializeWiFi(){ //This function initializes the WiFi module and connects to the network defined in arduino_secrets.h

  //Network info
  char ssid[] = SECRET_SSID;    // your network SSID (name)
  char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

  int status = WiFi.status(); //Checks status of WiFi
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

  ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");

}

//This function determines if the Arduino is running off of main power or the LiPo battery
//Code taken from https://forum.arduino.cc/t/mkr1010-how-to-detect-active-power-supply/1076615/3
void PowerSourceStatus(){

  //This is used to determine when the Arduino has switched to using battery power.
  #define PMIC_ADDRESS 0x6B

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
    LiPoFlag = false;
  } 
    
  else {
    LiPoFlag = true; //This flag will be sent out to indicates Arduino's power status
  }

}

void WiFiStatus(){ //This function attempts to reconnect the Arduino if the connection is dropped. NEEDS TESTING.

  //Network info
  char ssid[] = SECRET_SSID;    // your network SSID (name)
  char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

  status = WiFi.status(); //Checks status of WiFi
  
  delay(100);

  if(status == WL_CONNECTED){
    //Do nothing, get out of function
    //Return some statement that indicates the Arduino is connected
    //If statement isn't returned by some specified time or Arduino stops sending statement, we know Arduino is no longer connected to WiFi.

    //Once reconnection is established, dump SD card data to API.

    return;
  }

  else{

    int reconnectTries = 0; //Number of times the Arduino attempts to reconnect
    while ((status != WL_CONNECTED) && reconnectTries < 2) {

      reconnectTries++;
      Serial.println("Attempting to reconnect to WPA SSID: ");
      Serial.println(ssid);
      // Connect to WPA/WPA2 network:
      status = WiFi.begin(ssid, pass);

      // wait 10 seconds for connection:
      delay(10000);
    }
  }
}

void httpCommand() { //This function determines what kind of command was sent to the Arduino.

  if (Serial.available() > 0) {
    command = Serial.read();
    Serial.println(command);

      if (command == 'A'){
      readingActive = true; //Could be used to flag that the reading is happening
      Serial.println("Sensor and relays activated");
      rcReading();
      readingActive = false; 
    }
      if (command == 'B'){ //Used for testing purposes
      Serial.println(dataString);
      //getTime();
    }
      if (command == 'C'){
      Serial.println("Saving data to SD card...");
      saveDataToSD();
    }

    if (command == 'D'){

      readingActive = true;

      Serial.println("1");
      relaySet1();
      delay(1000);

      Serial.println("2");
      relaySet2();
      delay(1000);

      Serial.println("3");
      relaySet3();
      delay(1000);
   
      readingActive = false;
      relaySet1();
      relaySet2();
      relaySet3();
    
    }

    if (command == 'E'){
      readingActive = true; //Could be used to flag that the reading is happening
      Serial.println("Sensor and relays activated");
      lcReading();
      readingActive = false; 
    }
  }
}

void autoRead(){ //This function automatically takes readings at specific time periods. This period can be changing the readPeriod variable.

  currentMillis = millis();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMillis - startMillis >= readPeriod){  //test whether the period has elapsed
    readingActive = true;

    rcReading();
    lcReading();

    readingActive = false;
    startMillis = currentMillis;  //IMPORTANT to save the start time of the current LED state.
  }

}

