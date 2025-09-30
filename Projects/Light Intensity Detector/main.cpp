/*
;--------------------------------------
;Title: Light Intensity Meter
;--------------------------------------
; Program Details:
;--------------------------------------
; Purpose: This prgram is to measure the light intensity using a photoresistor and
;          calibrate a buzzer and an RGB LED to indicate the light intensity level.
; Inputs: The inputs of the program are the light intensity and the user serial input for the buzzer calibration
; -- Input 1: Analog signal - This input is what the photoresistor reads
; -- Input 2: B - This input makes the buzzer calibration start
; Outputs: A light intensity meter device that uses a buzzer and an RGB LED to indicate the light intensity level
; Date: 09/28/2025 09:58pm
; Compiler: Simulator Version 2.3.6
; Author: Juan Ali Ruiz Guzman
; Versions:
;               V1 - First version writing the program
;               V1.2 - Fixed syntax errors to compile
;               V2 - modified the code to work properly with a photoresistor, buzzer and RGB LED
;             V2.1 - Added comments and serial print statements to make the program more user friendly
;               V2.2 - Modified code for sensor calibration and RGB LED color ranges
;               V2.3 - Modified code to output voltage and estimated lux values
*/
//--------------------------------------
//File Dependencies: 
//--------------------------------------
#include <Arduino.h>
//--------------------------------------
// Main Program:
//--------------------------------------


const int buzzerPin = D5;          // Buzzer signal pin
const int redPin = D6;         // Red LED pin
const int greenPin = D7;       // Green LED pin
const int bluePin = D8;      // Blue LED pin


unsigned long lastReport = 0;
unsigned long reportInterval = 1000; // print every 1s

// Threshold for brightness
const int luxThresholdlow = 300;       // adjust after calibration
const int luxThresholdhigh = 2400; // absolute max of sensor
const float a = (3080-1.24)/(999-82);   // lux per ADC count
const float b = 1.24 - a*82;   // offset
const float m = 9.15e-4;      // slope for extimated lux to voltage
const float c = 0.361; // y-intercept for extimated lux to voltage


void setup() {
  Serial.begin(115200);  // Start serial communication at 115200 baud
  delay(1500);  // Allows time for serial to latch
  Serial.println("Light sensor meter starting...");  // user greeting before input option.

  pinMode(buzzerPin, OUTPUT); // Set buzzer pin as output
  digitalWrite(buzzerPin, LOW);    // buzzer off

  pinMode(redPin, OUTPUT);  // Set RGB pins as outputs
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
}

void setRGB(uint8_t r, uint8_t g, uint8_t b) { // Helper function to set RGB LED color
  analogWrite(redPin, r); //This is to allow the analog signal to control the brightness of each color
  analogWrite(greenPin, g);
  analogWrite(bluePin, b);
}

void loop() {
  // --- read sensor value ---
  int sensorValue = analogRead(A0);          // raw 0–1023
  sensorValue = 1023 - sensorValue; // invert, so higher value = more light
  // Lux calibration offset
  float lux = a*sensorValue + b; // rough calibration, adjust as needed 
  float voltage = (sensorValue / 1023.0) * 3.3;      //ESP voltage output
  float estLux = (voltage - c) / m; // estimated lux from voltage        

  // --- print reading every second ---
  if (millis() - lastReport >= reportInterval) { // checks if it's been too long since last report
    Serial.printf("Voltage: %.1f V\n ", voltage); // print the raw ADC value to serial 
    Serial.printf("Measured Lux: %.1f\n", lux); // print the lux value to serial if it's time to report again
    Serial.printf("Estimated Lux: %.1f\n", estLux); // print the estimated lux value to serial if it's time to report again
    lastReport = millis(); // update last report time
  }

  // --- react to threshold ---
  if (lux < luxThresholdlow) {
    // dim → activate buzzer
    digitalWrite(buzzerPin, HIGH);
    setRGB(255,255,255);     // white
  }else if(lux < 450) {// bright → silence buzzer and change LED color
    setRGB(255, 200, 200);       // pinkish
    digitalWrite(buzzerPin, LOW);
  }
  else if(lux < 600) {
    setRGB(255,120,120);       // light red
    digitalWrite(buzzerPin, LOW);
  } else if(lux < 800) {
    setRGB(255,60,60);       // medium red
    digitalWrite(buzzerPin, LOW);
    
  } else if(lux < 1200) {
    setRGB(255,50,30);       // strong red
    digitalWrite(buzzerPin, LOW);
  }
  else if(lux < 1800) {
    setRGB(255,30,15);       // very strong red
    digitalWrite(buzzerPin, LOW);
  }
  else if(lux < luxThresholdhigh) {
    setRGB(255,30,0);       // full red
    digitalWrite(buzzerPin, LOW);
  } else {
    // very bright → full red
    setRGB(255,0,0);       // full red
    digitalWrite(buzzerPin, LOW);
  }

  // --- test buzzer on command ---
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'B' || c == 'b') {
      Serial.println("Buzzer test for 5 seconds...");
      digitalWrite(buzzerPin, HIGH);
      delay(5000);
      digitalWrite(buzzerPin, LOW);
    }
  }
}

