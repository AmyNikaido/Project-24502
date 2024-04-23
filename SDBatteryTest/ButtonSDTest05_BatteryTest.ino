//Test File for toggling SD card writing on main power loss

#include <SPI.h>
#include <SD.h>
#include <Wire.h>

#define PMIC_ADDRESS 0x6B

// constants won't change. They're used here to set pin numbers:
const int chipSelect = 4; //Pin for SD card

File dataFile;

// Variables will change:
int counter = 0;
int ledState = 1;                 // the current state of the LED pin
int SDToggle = 0;

void setup() {


  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    //Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  //Serial.println("card initialized.");

  if (SD.exists("datalog.txt")) {
    //Serial.println("Removing datalog.txt...");
    SD.remove("datalog.txt");
  }

  //Initialize the pinmodes for the button and built in LED
  Wire.begin();

  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {

  // make a string for assembling the data to log:
  String dataString = "";
  // read the state of the switch into a local variable:

  counter++;
  delay(1000);
  //dataString += String(counter);

  //File is opened
  dataFile = SD.open("datalog.txt", FILE_WRITE);

  byte reg, val;
  // reading all registers
  //  byte BQ_REG[10] = { 0x00, 0x01, 0x2, 0x03, 0x04, 0x05, 0x06,0x07,0x08, 0x09 };

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

  if (val == 4) {
    digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on if reg8 bit 2 is true
    ledState = 1;
  } 
  else {
    digitalWrite(LED_BUILTIN, LOW);
    ledState = 0;  
  }


  //SD toggle tied to state of LED
  if (ledState == 0) {
    dataString += String(counter);
    dataFile.println(dataString);
    dataFile.close();
  }


  // save the current state as the last state, for next time through the loop
}

