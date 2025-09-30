/*
;--------------------------------------
;Title: Modifiable LED Blink
;--------------------------------------
; Program Details:
;--------------------------------------
; Purpose: This prgram controlls the blinking of an LED whose speed can be controlled via user serial input
; Inputs: The inputs of the program are the speeds sent through serial by the user
; -- Input 1: A - This input makes the LED blink Faster
; -- Input 2: B - This input makes the LED blink Slower
; Outputs: A madofiable blink rate signal that controls the blink rate of an LED
; Date: 09/28/2025 01:58pm
; Compiler: Simulator Version 2.3.6
; Author: Juan Ali Ruiz Guzman
; Versions:
;               V1 - First version writing the program
;               V1.2 - Fixed syntax errors to compile
;               V2 - modified the code to be (non-blocking by using toggles vs previously using delayed on-off functions
;               V2.1 - Added comments and serial print statements to make the program more user friendly
;               V2.2 - Modified the code to work with PlatformIO and print LED status after every change
;
;
*/
//--------------------------------------
//File Dependencies: 
//--------------------------------------
#include <Arduino.h>
//--------------------------------------
// Main Program:
//--------------------------------------

unsigned long BlinkInterval = 1000; // Speed interval
unsigned long lastToggleTime = 0;
bool ledState = false;

void printLedStatus() {
  //Print the status of the LED, either ON or OFF
  Serial.printf("LED is %s (interval: %lu ms)\n", ledState ? "ON" : "OFF" , BlinkInterval);
}
// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200); //match the board baude rate
  delay(1500); //Allows time for serial to latch
  Serial.println("Blink control started. "); //user greeting before input option.
  Serial.println("Send 'A' to blink faster, 'B' to blink slower.");
  //Print initial status
  printLedStatus();
}

// the loop function runs over and over again forever
void loop() {
  // Handle Serial Input
  if (Serial.available() > 0) {
    char c = Serial.read(); //char is used because we only read one character

    if (c == 'A' || c == 'a') { //If the user inputs A, then blink faster
    BlinkInterval = (BlinkInterval >50) ? BlinkInterval / 2 : 50; //If BlinkInterval is > 50, then divide by 2, otherwise set it to 50
    Serial.printf("Speed is now 2x fast. Interval: %lu ms\n", BlinkInterval);
    } else if (c == 'B' || c == 'b') { //If the user inputs B, then blink slower
    BlinkInterval = (BlinkInterval < 5000) ? BlinkInterval * 2: 5000; //If BlinkInterval is <5000, then multiply by 2, otherwise set to 5000
    Serial.printf("Speed is now 0.5x fast. Interval: %lu ms\n", BlinkInterval);
    }
  }
  //Blink the LED based on interval
  unsigned long now = millis(); //millis() returns the number of milliseconds since the ESP has powered on/reset. This is a good way to keep track of timing in a non-blocking way
  if(now - lastToggleTime >= BlinkInterval) { //has the elapsed time become greater than or equal to the current BlinkInterval? 
    ledState = !ledState; //If it's reached the time then toggle LED
    digitalWrite(LED_BUILTIN, ledState ? LOW : HIGH);
    lastToggleTime = now; //sets the last time the LED was toggled to the current time since powerup of ESP
    printLedStatus();

  }
}