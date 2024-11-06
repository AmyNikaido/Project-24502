/*

Team 24502: Wireless Sample Mode Response Technique Sensor for Power Supplies and HVAC Systems

*/
#include <WiFiNINA.h>
#include <SPI.h>
#include <SD.h>
#include <string.h>
#include <TimeLib.h>
#include <Wire.h>
#include <ArduinoLowPower.h> //NEED TO IMPLEMENT LOW POWER FUNCTIONANILITY LATER
#include <ArduinoHttpClient.h>
#include "arduino_secrets.h"

//#define sampleAmt 500  //This will allow you to change the sample amount quickly without having to scroll through the code
//#define readResolution 12  //This will allow you to change the read resolution quickly without having to scroll through the code

//This may not work 100% but worth a try if we need to edit quickly
//#define voltDecayDiv 4096
//#define ringDiv 4.096

// Constants that won't change:
const int ledPin = LED_BUILTIN;    // the number of the LED pin
const int RELAY_PIN1 = 3;  // the Arduino pin, which connects to the IN pin of the second pair of relays. This pair of relays cuts off the capacitor/resistor combo from the power source.
const int RELAY_PIN2 = 4;  // the Arduino pin, which connects to the IN pin of the first pair of relays. This pair of relays controls the sensor input/sensing.
//const int RELAY_PIN3 = 5; // the Arduino pin, which connects to the IN pin of the third pair of relays. This pair of relays controls the RingDown input/sensing.
const int analogPin = 16; // A1, the Arduino pin that will read voltage
//const int analogPin2 = 17; //A2
const int pulsePin = 20; //A5, the Arduino pin that will pulse in a voltage to the capacitor
const int chipSelect = 4; //Pin that will be used for the SD card

// Sets read resolution to 12 bit - VoltDiv = 2^(Resolution) - 3.3/VoltDiv
//const int readResolution = 12;

// Variables that will change:
bool readingActive = false; //This bool variable determines if the sensing process is active or not.
bool LiPoFlag = false; //If false, Arduino is not using LiPo power. Else, battery is in use. 

char serverAddress[] = "192.168.0.3";  // server address
char uploadEndpoint[] = "/upload";
IPAddress ip;
int port = 8080;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

// String for data
String dataString = "";

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
  //ADC->CTRLA.bit.ENABLE = 0;                    // Disable ADC
  //while(ADC->STATUS.bit.SYNCBUSY == 1);         // Wait for synchronization
  //ADC->CTRLB.reg = ADC_CTRLB_PRESCALER_DIV64;   // Divide Clock by 512.
  //ADC->AVGCTRL.reg = ADC_AVGCTRL_SAMPLENUM_1 | ADC_AVGCTRL_ADJRES(0x00ul);  // 1 sample | Adjusting result by 0
  //ADC->SAMPCTRL.reg = 0x00;                     // Sampling Time Length = 0
  //ADC->CTRLA.bit.ENABLE = 1;                    // Enable ADC
  //analogReadResolution(readResolution);

  // Uses internal voltage reference for sampling
  //analogReference(AR_INTERNAL);
    
  Serial.begin(9600);
  Wire.begin();
  
  initializeSD(); //Initializes the SD card module
  initializeWiFi(); //Initializes WiFi

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
  
  PowerSourceStatus(); //checks power status

  WiFiStatus(); //checks WiFi status
  
  httpCommand(); //Arduino waits to receive call for data collection

}

//Based off this post https://forum.arduino.cc/t/controlling-relay-with-an-http-request/164746
//Will need to test to fully integrate with our API
void httpCommand() { //This function determines what kind of command was sent to the Arduino.

  String readString;

  if(!client){
    return; //Print out some error that client is not available
  }

  if(!client.connected()){
    return; //Print out some error
  }

  if(!client.available()){
    return; //Print out some error
  }

  char c = client.read();

  //read char by char HTTP request
  if (readString.length() < 100) {
    //store characters to string 
    readString += c; 
    //Serial.print(c);
  } 

  //if HTTP request has ended
  if (c == '\n') {

    Serial.println(readString); //print to serial monitor for debuging 

    client.println("HTTP/1.1 200 OK"); //send new page
    client.println("Content-Type: text/html");
    client.println();

    client.println("<HTML>");
    client.println("<HEAD>");
    client.println("<TITLE>Arduino GET test page</TITLE>");
    client.println("</HEAD>");
    client.println("<BODY>");

    //client.println("<H1>Zoomkat's simple Arduino button</H1>");
          
    client.println("<a href=\"/?on\">ON</a>"); 
    client.println("<a href=\"/?off\">OFF</a>"); 

    client.println("</BODY>");
    client.println("</HTML>");
 
    delay(1);
    //stopping client
    client.stop();

  }

  if(readString.indexOf("rc") >0){

    rcReading();

  }

  //clearing string for next read
  readString="";

}

void relaySet1(){ //This function activates the first pair of relays that will cut off the capacitor and resistor combo from the main power source.

  const long bleedInterval = 20000;  //Interval or time it takes for the capacitor to bleed out to near 0V. This number is obtained from the capacitor bleed equation.
  
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

/* Input for relay set 3. you can edit this however you'd like for logic, but looks like it's similar to the others
void relaySet3(){ //This function activates the second pair of relays that will connect the arduino to the resistor and capacitor, and allow it to pulse a voltage and take readings

  if (readingActive == true){
    digitalWrite(RELAY_PIN3, LOW); 
  }

  else if(readingActive == false){
    digitalWrite(RELAY_PIN3, HIGH); 
  }
}
*/
void rcReading(){ //This function is the RC decay functionality

    relaySet1();
    relaySet2();
    sampleDataAndToString();
    saveDataToSD();
    delay(1000);
    relaySet2();
    delay(1000);
    relaySet1();

}

// Possibly change this to reflect it being for voltage decay and split things 
void sampleDataAndToString(){ //This function pulses in 3.3 volts into the capacitor then samples the decaying voltage into an array, takes readings from the analog pin, and saves that data into an array

  // These can probably go up top before the set up so we're not creating ints when we want to sample, not sure if its for http transfer though
  uint16_t sampleAmt = 500; // this can be replaced with the #define sampleAmt at the top
  uint32_t dataTime;
  uint32_t timeStart, timeFinish, timeTotal;
  uint32_t timeArray[500]; //Array that will contain the time of the sample data  //if using the sampleAmt: uint32_t timeArray[sampleAmt];
  uint16_t sampleArray[500]; // Array that will contain sample data  //if using the sampleAmt: uint16_t sampleArray[sampleAmt];

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

  Serial.println("data sampled");

  if(dataString.length() != 0){

    dataString = "";

  }

  for(int j = 0; j < sampleAmt; j++){

      //float voltage = sampleArray[j] * (3.3 / voltDecayDiv); // If using voltDecayDiv
      //float voltage = sampleArray[j] * (3.3 / 4096.0); // If voltDecayDiv doesn't work
      float voltage = sampleArray[j] * (3.3 / 1023.0); // this would need to go to 4096.0 for 12-bit samples
      
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

  readingActive = false;
}


void sampleRingDownDataAndToString(){ //This takes readings for the Ringdown circuit and puts it into an array
    
  delay(1000);  //Delay a second to make sure relays have switched and all is ready to sample
  timeStart = micros();
    for(uint16_t i = 0; i < sampleAmt; i++){
      digitalWrite(pulsePin,HIGH);  //Pin needs to stay high while sampling to get a good ring
      timeFinish = micros();
      timeTotal = timeFinish - timeStart;
      timeArray[i] = timeTotal;
      sampleArray[i] = analogRead(analogPin2);
    }
  digitalWrite(pulsePin,LOW);
  Serial.println("data sampled.");

  if(dataString.length() != 0){
    dataString = "";
  }

  for(int j = 0; j < sampleAmt; j++){

      //float voltage = sampleArray[j] * (3.3 / ringDiv); //If using ringDiv
      float voltage = sampleArray[j] * (3.3 / 4.096); // this needs to 4.096 so I can read at millivolts ~600mV
      dataTime = timeArray[j];

      dataString += dataTime;
      dataString += ",";
      dataString += voltage;
      dataString += ",";
      dataString += voltageInverse;
      dataString += "\n";

    }

  
}



void initializeSD(){ //This function initializes the SD card. If the SD card is not present, the Arduino will not function

  if (!SD.begin(chipSelect)) {// If an SD card is not present, do not initialize
    // don't do anything more:
    while (1);
  }

  Serial.println("SD card initialized");

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

  postDataToServer(fileName);

  dataFile.close();
  dataFile.flush();
  Serial.println("data saved.");

}

void DateTimeSD (uint16_t* date, uint16_t* time){ //This function adds a timestamp to the file that Windows will display in the file information

  *date = FAT_DATE(ts_year, ts_month, ts_day);
  *time = FAT_TIME(ts_hour, ts_minute, ts_second);

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
    digitalWrite(ledPin, HIGH);  // turn the LED on if reg8 bit 2 is true
    LiPoFlag = false;
  } 
    
  else {
    digitalWrite(ledPin, LOW);  
    LiPoFlag = true; //This flag will be sent out to indicates Arduino's power status

  }

}

void WiFiStatus(){ //This function attempts to reconnect the Arduino if the connection is dropped. NEEDS TESTING.

  //Network info
  char ssid[] = SECRET_SSID;    // your network SSID (name)
  char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

  int status = WiFi.status(); //Checks status of WiFi
  
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

void postDataToServer(const char* filename){

  //access the most recently made file and then upload

  //These parameters can be changed as needed
  char serverAddress[] = "192.168.0.3";  // server address
  char uploadEndpoint[] = "/upload";
  IPAddress ip;
  int port = 8080;

  File dataFile = SD.open(filename);

  if (!dataFile)
  {
    Serial.println("Failed to open file for sending");
    return;
  }

  // Calculate and send the Content-Length header
  unsigned long contentLength = dataFile.size(); // Get file size
  Serial.print("File size (Content-Length): ");
  Serial.println(contentLength);

  // Create the HTTP request
  if (client.connect(ip, port))
  {
    Serial.println("Connected to server, sending file...");

    // Send the HTTP POST request headers
    client.println("POST " + String(uploadEndpoint) + " HTTP/1.1");
    client.println("Host: " + String(ip));
    client.println("Content-Type: text/csv");
    client.print("Content-Length: ");
    client.println(contentLength);
    client.println("Connection: close");
    client.println();

    // Send the file content in small chunks
    const int bufferSize = 64; // Adjust buffer size based on your memory constraints
    char buffer[64];
 
    while (dataFile.available())
    {
      int bytesRead = dataFile.read(buffer, bufferSize); // Read up to bufferSize bytes
      client.write((const uint8_t*) buffer, bytesRead);                   // Send the buffer content to the server
    }

    dataFile.close(); // Close the file
    Serial.println("File content sent");

    // Wait for the response from the server
    while (client.connected())
    {
      if (client.available())
      {
        String response = client.readStringUntil('\n');
        Serial.println("Server response: " + response);
      }
    }

    client.stop(); // Close the connection
    delay(1000);   // Wait before the next request
  }
  else
  {
    Serial.println("Connection to server failed");
  }
}


/*
  if (command == 'A'){

    readingActive = true; //Could be used to flag that the reading is happening
    rcReading();
    Serial.println("Sensor and relays activated");
    readingActive = false; 

  }

  if (command == 'C'){

    Serial.println("Saving data to SD card...");
    saveDataSD();

  }*/
  

