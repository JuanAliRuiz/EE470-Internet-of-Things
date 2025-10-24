/*
;--------------------------------------
;Title: Sesnor Database, Programming and ESP Assignment for EE470
;--------------------------------------
; Program Details:
;--------------------------------------
; Purpose: This program calls on the classes defined in SimpleIot.hpp to read sensor data and send it to a database
; Inputs: The inputs of the program are the Butn and Tilt pins
; Outputs: The outputs of the program are the sensor readings sent to the database and serial outputs for debugging.
; Date: 10/20/2025 02:42pm
; Compiler: PlatformIO Version 5.2.0
; Author: Juan Ali Ruiz Guzman
; Versions:
;               V1 - First version writing the program
;               V2 - Added Function to aalow timezone selection via serial
;               V3 - Fixed minor bugs with WiFi connection and timezone selection
;               V4 - Final version before submission with comments added 
*/
//--------------------------------------
//File Dependencies: 
//--------------------------------------
#include <Arduino.h>
#include "SimpleIot.hpp"
#include <ESP8266WiFi.h>

// --- user settings ---
static const char* WIFI_SSID = "AliAppleiPhone";
static const char* WIFI_PASS = "Imaleechyes";
static String ENDPOINT = "https://juanaliruiz.com/db_insert_2.php";

// --- pins ---
constexpr int PIN_TRIG = D7;   // HC-SR04 TRIG
constexpr int PIN_ECHO = D8;   // HC-SR04 ECHO (level-shifted to 3.3V)
constexpr int PIN_LDR  = A0;   // Photoresistor divider
constexpr int PIN_BTN  = D5;   // External resistor network
constexpr int PIN_TILT = D6;   // External resistor network
// -------------

App* app;

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("\nESP8266 Simple IoT (Ultrasonic + Photoresistor)");

  // One call: shows TimeZone menu, builds App, brings up Wi-Fi
  app = createAppWithTimezoneAndWifi( 
    WIFI_SSID, WIFI_PASS, ENDPOINT,
    PIN_TRIG, PIN_ECHO, PIN_LDR, PIN_BTN, PIN_TILT,
    20000  // 20s timeout, defaults to PT if no input
  );

  Serial.println("Ready: BUTTON -> ultrasonic (Node_1), TILT -> photoresistor (Node_2).");
}//-- End setup --
//-- Main loop --
void loop() {
  app->tick(); // process nodes
  delay(30); // small delay to avoid busy loop
}
