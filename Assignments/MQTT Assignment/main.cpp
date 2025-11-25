/*
;--------------------------------------
;Title: Sesnor Database, Programming and ESP Assignment for EE470
;--------------------------------------
; Program Details:
;--------------------------------------
; Purpose: Publishes potentiometer value to HiveMQ every ~15 seconds
    - Subscribes to inTopic and controls LED based on MQTT messages
    - On switch press: publishes "1" immediately, then "0" after 5 seconds
; Inputs: The physcal inputs are a potentiometer connected to the ADC pin (A0) and a momentary switch connected to a digital pin (D5).
; Outputs: The outputs are MQTT messages published to the HiveMQ broker and an LED connected to a digital pin (D4) that is controlled via MQTT messages.
; Date: 11/24/2025 11:33 PM
; Compiler: PlatformIO Version 5.2.0
; Author: Juan Ali Ruiz Guzman
; Versions:
;               V1 - First version writing the program
;               V2 - Final version with comments and testing
*/

//--------------------------------------
//File Dependencies: 
//--------------------------------------
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// -------------------- USER CONFIG --------------------

// TODO: change to your Wi-Fi credentials
const char* ssid     = "AliAppleiPhone";
const char* password = "(put actual password here)";

// MQTT broker (HiveMQ public broker)
const char* mqtt_server = "broker.hivemq.com";
const int   mqtt_port   = 1883;

// Topics
// outTopic: where the potentiometer and switch states are published
const char* outTopic = "testtopic/temp/outTopic/PotentiometerAli";

// inTopic: where you listen for LED commands
const char* inTopic  = "testtopic/temp/inTopic";

// -----------------------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

// Pins
const int potPin    = A0;   // ADC pin
const int ledPin    = D4;   // LED pin
const int switchPin = D5;   // Switch pin (INPUT_PULLUP)

// Pot publish timing
unsigned long lastPublish = 0;
const unsigned long publishInterval = 15000;  // 15 seconds

// Switch handling
bool switchPressed      = false;
unsigned long pressTime = 0;
int lastSwitchState     = HIGH;  // because of INPUT_PULLUP

// -------------------- Wi-Fi Setup --------------------
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// -------------------- MQTT Callback --------------------
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to String
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }

  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(msg);

  // Check if it's for our LED control topic
  if (String(topic) == inTopic) {
    if (msg == "1") {
      digitalWrite(ledPin, HIGH);
      Serial.println("LED turned ON from MQTT");
    } else if (msg == "0") {
      digitalWrite(ledPin, LOW);
      Serial.println("LED turned OFF from MQTT");
    } else {
      Serial.println("Unknown payload for LED, expected '1' or '0'");
    }
  }
}

// -------------------- MQTT Reconnect --------------------
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection... ");

    // Create a client ID based on chip ID to make it unique-ish
    String clientId = "ESP8266Client-";
    clientId += String(ESP.getChipId(), HEX);

    // Attempt to connect (no username/password for public broker)
    if (client.connect(clientId.c_str())) {
      Serial.println("connected!");
      // Subscribe to inTopic so we can receive LED commands
      client.subscribe(inTopic);
      Serial.print("Subscribed to: ");
      Serial.println(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" - try again in 5 seconds");
      delay(5000);
    }
  }
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(potPin, INPUT);

  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);

  pinMode(switchPin, INPUT_PULLUP);

  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// -------------------- Main Loop --------------------
void loop() {
  // Maintain MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  unsigned long now = millis();

  // ---------- Part B: Publish pot value every 15 seconds ----------
  if (now - lastPublish >= publishInterval) {
    lastPublish = now;

    int adcValue = analogRead(potPin);  // 0–1023
    char payload[16];
    snprintf(payload, sizeof(payload), "%d", adcValue);

    bool ok = client.publish(outTopic, payload);

    Serial.print("Publishing POT to [");
    Serial.print(outTopic);
    Serial.print("] value: ");
    Serial.print(payload);
    if (ok) {
      Serial.println("  -> OK");
    } else {
      Serial.println("  -> FAILED");
    }
  }

  // ---------- Part C: Switch press -> publish 1, then 0 after 5s ----------

  int currentSwitchState = digitalRead(switchPin);

  // Detect falling edge (HIGH -> LOW) because of INPUT_PULLUP
  if (lastSwitchState == HIGH && currentSwitchState == LOW) {
    // Switch just pressed
    switchPressed = true;
    pressTime = now;

    // Publish "1" immediately
    client.publish(outTopic, "1");
    Serial.println("Switch pressed: published '1' to outTopic");
  }
  lastSwitchState = currentSwitchState;

  // After 5 seconds from press, publish "0" once
  if (switchPressed && (now - pressTime >= 5000)) {
    client.publish(outTopic, "0");
    Serial.println("5 seconds elapsed: published '0' to outTopic");
    switchPressed = false;
  }
}
