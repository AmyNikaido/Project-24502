//Test File for toggling SD card writing on WiFi loss
#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <WiFiNINA.h>

#include "SDWiFi.h"
#include "arduino_secrets.h"


char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;     // the Wifi radio's status

File dataFile;

// constants won't change. They're used here to set pin numbers:
const int chipSelect = 4; //Pin for SD card
const int WiFiTogglePin = 7;  // the number of the pushbutton pin

// Variables will change:
int counter = 0;
int ledState = 1;                 // the current state of the LED pin
int WiFiToggleState = 0;            // the current reading from the WiFitoggle pin
int lastWiFiToggleState = 0;        // the previous reading from the WiFitoggle pin
int WiFiToggle = 0;

void setup() {

  Serial.begin(9600);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1);
  }
  Serial.println("card initialized.");

  if (SD.exists("datalog.txt")) {
    Serial.println("Removing datalog.txt...");
    SD.remove("datalog.txt");
  }


  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }

  //Initialize the pinmodes for the button and built in LED
  Wire.begin();
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(WiFiTogglePin, INPUT_PULLUP);

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");
  Serial.println("----------------------------------------");

  Serial.println(WL_CONNECTED);
  Serial.println(WL_DISCONNECTED);
  Serial.println(WL_CONNECTION_LOST);
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

  // read the state of the switch into a local variable:
  WiFiToggle = digitalRead(WiFiTogglePin);

   // if the button state has changed:
  if (WiFiToggle != lastWiFiToggleState){
    WiFiToggleState = WiFiToggle;
    //If the state is HIGH then the SD card has been toggled on and file is opened
    //SD card tied to state of the LED
    if (WiFiToggleState == HIGH) {
    WiFi.disconnect();
    }
    //Delay to avoid bouncing
    delay(50);
  }

  // set the LED:
  digitalWrite(LED_BUILTIN, ledState);

  if(status != WL_CONNECTED){
    ledState = 0;
  }

  
  if (ledState==0) {
    dataString += String(counter);
    dataFile.println(dataString);
    dataFile.close();
  }

  // save the current state as the last state, for next time through the loop
  lastWiFiToggleState = WiFiToggle;
  status = WiFi.status();
  delay(1000);
}



