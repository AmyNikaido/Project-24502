#include <WiFiNINA.h>
#include <RTCZero.h>
#include <SPI.h>
#include <SD.h>         // Add the SD card library
#include <WiFiClient.h> // WiFiClient for HTTP POST requests

// WiFi credentials
const char *SSID = "TP-Link_BE10";      // Your Wi-Fi SSID
const char *PASSWORD = "SensorLab123$"; // Your Wi-Fi password

// Server details
const char *SERVER_IP = "192.168.0.86";  // IP address of your Raspberry Pi
const int SERVER_PORT = 5000;            // Port of your Flask server
const char *UPLOAD_ENDPOINT = "/upload"; // Flask upload endpoint

// Function prototypes
void sampleData();
void printToSerial();
void dataToString();
void saveToSDCard();     // Function to save data to SD card
void sendFileToServer(); // Function to send data file to the server

String dataString = "";
int readResolution = 12;
int analogPin = A1; // Use A1 for analog pin
int OutputPin = A5; // Use A5 for digital output pin
unsigned int sampleAmt = 499;
unsigned int sampleArray[499];
const int chipSelect = 4; // SD card chip select pin (adjust depending on your board)
int fileCounter = 0;      // Counter to create unique filenames

void connectWiFi()
{
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(SSID, PASSWORD);
    delay(500);
  }
  Serial.println("Connected to WiFi!");
}

WiFiClient client;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ; // Wait for serial port to connect
  }

  connectWiFi();

  // Initialize the SD card
  if (!SD.begin(chipSelect))
  {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized.");

  analogReference(AR_INTERNAL);
  analogReadResolution(readResolution);
  pinMode(OutputPin, OUTPUT);
}

void loop()
{
  sampleData();
  dataToString();
  printToSerial();
  saveToSDCard();     // Save the data to the SD card as a .csv file
  sendFileToServer(); // Send the data file to the server

  delay(10000); // Delay to avoid spamming the server, adjust as needed
}

void sampleData()
{
  digitalWrite(OutputPin, HIGH);
  delay(1000);
  digitalWrite(OutputPin, LOW);
  for (int i = 0; i < sampleAmt; i++)
  {
    sampleArray[i] = analogRead(analogPin);
  }
}

void printToSerial()
{
  for (int j = 0; j < sampleAmt; j++)
  {
    float voltage = sampleArray[j] * (3.3 / 1023.0);
    Serial.println(voltage);
  }
}

void dataToString()
{
  dataString = ""; // Clear the previous data
  // Add the header if necessary (remove this if not needed)
  dataString += "Voltage,Time(ms)\n";

  for (int j = 0; j < sampleAmt; j++)
  {
    float voltage = sampleArray[j] * (3.3 / 4096);
    // Format the data as CSV: voltage,time
    dataString += String(voltage) + "," + millis() + "\n";
  }
}

// Function to save the sampled data to the SD card as a .csv file with a unique filename
void saveToSDCard()
{
  // Generate a unique filename using a counter
  String filename = "data_" + String(fileCounter) + ".csv";
  fileCounter++; // Increment the counter for the next file

  // Open the file on the SD card
  File dataFile = SD.open(filename.c_str(), FILE_WRITE);

  if (dataFile)
  {                               // If it opens
    dataFile.println(dataString); // Write the dataString to the file
    dataFile.close();             // Close the file
    Serial.println("Data saved to SD card: " + filename);
  }
  else
  {
    Serial.println("Error opening " + filename);
  }
}

// Function to send the .csv file to the server
void sendFileToServer()
{
  // Generate the filename again (make sure it matches the one created in saveToSDCard)
  String filename = "data_" + String(fileCounter - 1) + ".csv"; // Get the last saved file
  File dataFile = SD.open(filename.c_str());

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
  if (client.connect(SERVER_IP, SERVER_PORT))
  {
    Serial.println("Connected to server, sending file...");

    // Send the HTTP POST request headers
    client.println("POST " + String(UPLOAD_ENDPOINT) + " HTTP/1.1");
    client.println("Host: " + String(SERVER_IP));
    client.println("Content-Type: text/csv");
    client.print("Content-Length: ");
    client.println(contentLength);
    client.println("Connection: close");
    client.println();

    // Send the file content in small chunks
    const int bufferSize = 64; // Adjust buffer size based on your memory constraints
    char buffer[bufferSize];
    while (dataFile.available())
    {
      int bytesRead = dataFile.read(buffer, bufferSize); // Read up to bufferSize bytes
      client.write(buffer, bytesRead);                   // Send the buffer content to the server
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
