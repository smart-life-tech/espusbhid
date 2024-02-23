#include <WiFi.h>        // For connecting ESP32 to WiFi
#include <ArduinoOTA.h>  // For enabling over-the-air updates

//const char* ssid = "Your SSID";  // Change to your WiFi Network name
//const char* password = "You Password";  // Change to your password
const char *ssid = "XCREMOTE";
// const char* ssid = "xcremote";
const char *password = "xcremote";
void setup() {

  WiFi.begin(ssid, password);  // Connect to WiFi - defaults to WiFi Station mode

  // Ensure WiFi is connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  ArduinoOTA.begin();  // Starts OTA
}

void loop() {

  ArduinoOTA.handle();  // Handles a code update request

  // All loop you're code goes here.
}