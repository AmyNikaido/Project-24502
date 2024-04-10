
int reconnectWiFi(char ssid[], char pass[]){

    int status = WL_IDLE_STATUS;     // the Wifi radio's status

    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);

    return status;
}
