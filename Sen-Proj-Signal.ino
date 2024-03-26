// Pins for MKR Wi-Fi 1010
int OutputPin = 20;
int analogPin = 16;
int sampleVal = 0; // Initial value for samples
int sampleAmt = 1000; // Amount of samples to be taken and sent to serial port

/* Sets pin to an output signal and Begins reading data and porting it to serial monitor 
at 115200 bps (can change later for whatever needs different rates) */
void setup() {
  pinMode(OutputPin,OUTPUT); // Set pin for output
  Serial.begin(115200); // Baud rate for data to serial monitor
}

// Loop delays 3 seconds to allow system start up before immediately taking samples 
void loop() {
  delay(3000); // 3 second delay
  digitalWrite(OutputPin,HIGH); // Sets digital pin to output
  delay(1000); // Output pin is HIGH for 1 sec

  // Loop for taking samples, converting it to a proper format (voltage) then sends to serial port
  for(int i = 0; i < sampleAmt; i++){
    int capLevel = analogRead(analogPin); // Reads level from ADC pin
    float voltage = capLevel * (3.3/1024); // Converts level to voltage. 3.3V output from pins for MKR Wi-Fi 1010
    Serial.println(voltage); // Prints voltage level to serial port as value
    digitalWrite(OutputPin,LOW); // Sets output pin to low.
    // -------------- READ BELOW ----------- //
    /* Putting digitalWrite() outside of the loop was an issue, as the voltage would immediately decay
     and wouldn't give a good initial reading of ~3.3V. May need to fix this in some way.
    */
  }
  exit(0);
}
