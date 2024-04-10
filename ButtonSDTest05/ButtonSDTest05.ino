//Test File for toggling SD card writing

#include <SPI.h>
#include <SD.h>

// constants won't change. They're used here to set pin numbers:
const int SDTogglePin = 7;  // the number of the pushbutton pin
const int chipSelect = 4; //Pin for SD card

File dataFile;

// Variables will change:
int counter = 0;
int ledState = 0;                 // the current state of the LED pin
int SDToggleState = 0;            // the current reading from the SDtoggle pin
int lastSDToggleState = 0;        // the previous reading from the SDtoggle pin
int SDToggle = 0;

void setup() {

  // Open serial communications and wait for port to open:
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

  //Initialize the pinmodes for the button and built in LED
  pinMode(SDTogglePin, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

}

void loop() {

  // make a string for assembling the data to log:
  String dataString = "";
  // read the state of the switch into a local variable:
  SDToggle = digitalRead(SDTogglePin);

  counter++;
  delay(1000);
  //dataString += String(counter);

  //File is opened
  dataFile = SD.open("datalog.txt", FILE_WRITE);

   // if the button state has changed:
  if (SDToggle != lastSDToggleState){
    SDToggleState = SDToggle;
    //If the state is HIGH then the SD card has been toggled on and file is opened
    //SD card tied to state of the LED
    if (SDToggleState == HIGH) {
    ledState = !ledState;
    }
    //Delay to avoid bouncing
    delay(50);
  }

  // set the LED:
  digitalWrite(LED_BUILTIN, ledState);

  //SD toggle tied to state of LED
  if (!ledState) {

    dataString += String(counter);
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }

  // save the current state as the last state, for next time through the loop
  lastSDToggleState = SDToggle;
}

