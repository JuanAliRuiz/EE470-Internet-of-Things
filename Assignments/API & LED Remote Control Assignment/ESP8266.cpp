/*
;-------------------------------------------------
; Title: Button Push For API Pull and LED CONTROL
;-------------------------------------------------
; Program Details:
;-------------------------------------------------
; Purpose:
;   This program connects an ESP8266 microcontroller to a Wi-Fi network and 
;   allows the user to interact with cloud-based APIs using button inputs.
;   One button sends a JSON-formatted PUT request to an IFTTT webhook to 
;   trigger a Slack notification, while another button performs a GET request 
;   to retrieve the current LED and RGB states from a Hostinger web server.
;   The ESP8266 updates its physical LED and RGB outputs based on the data 
;   received from the API in real time.
;
; Inputs:
;   - BTN1 (D3): Sends IFTTT PUT request (Slack notification trigger)
;   - BTN2 (D4): Fetches LED/RGB state from Hostinger server (manual polling)
;   - Wi-Fi credentials for network connection
;
; Outputs:
;   - LED1 (D2): ON/OFF control via API state
;   - RGB LED (D5: Red, D6: Green, D7: Blue): Color intensity via JSON data
;   - Serial Monitor messages for debugging and status feedback
;   - Slack message (via IFTTT webhook)
;
; Date: 11/03/2025 4:00 PM
; Compiler: PlatformIO (ESP8266 Core v3.x)
; Author: Juan Ali Ruiz Guzman
;
; Versions:
;   V1.0  - First version writing the program
;   V1.2  - Fixed syntax errors to compile successfully
;   V1.3  - Added manual polling buttons and secure HTTPS requests
;   V1.4  - Combined IFTTT trigger and API-controlled LED/RGB integration
;   V1.5  - Implemented button cooldown and buffered IFTTT retry mechanism
*/
//--------------------------------------
//File Dependencies: 
//--------------------------------------
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <ArduinoJson.h>

// ---- WiFi & URL ----
const char* WIFI_SSID = "AliAppleiPhone";
const char* WIFI_PASS = "Imaleechyes";


// Server endpoints (NOTE: capital /API)
const char* URL_IFTTT = "https://juanaliruiz.com/API/ifttt_put.php"; // PUT JSON
const char* URL_READ  = "https://juanaliruiz.com/API/read_state.php"; // GET JSON

// ===== ESP8266 unified sketch: Part 1B + 2A + 2B (button-only networking) =====
// BTN1 -> send IFTTT message   |   BTN2 -> poll LED/RGB state from server

// ---------- HARDWARE SETUP ----------
// GPIOs
#define LED1_PIN   D2      // physical single LED
#define RGB_R_PIN  D5
#define RGB_G_PIN  D6
#define RGB_B_PIN  D7

#define BTN_SEND   D3      // BTN1: send IFTTT message (to GND)
#define BTN_POLL   D4      // BTN2: poll server & update LEDs (to GND)

// Hardware type for RGB
const bool COMMON_ANODE = false;   // set true if your RGB is common-anode

// Button debounce & cooldown
const unsigned long DEBOUNCE_MS  = 30;
const unsigned long COOLDOWN_MS  = 800;  // per-button

// ---------- INTERNAL STATE ----------
struct BtnState {
  bool last = true;                 // pull-up, idle HIGH
  unsigned long lastEdgeMs = 0;
  unsigned long lastActionMs = 0;   // cooldown tracking
};
BtnState btnSend, btnPoll;

// One-message buffer for IFTTT if a send fails
String pendingIFTTT = "";

// ---------- Avoiding issues with button presses ----------
bool justPressed(int pin, BtnState& s) {
  bool cur = digitalRead(pin);
  unsigned long now = millis();
  if (cur != s.last && (now - s.lastEdgeMs) > DEBOUNCE_MS) {
    s.lastEdgeMs = now; s.last = cur;
    if (cur == LOW && (now - s.lastActionMs) >= COOLDOWN_MS) {
      s.lastActionMs = now;
      return true;
    }
  }
  return false;
}

std::unique_ptr<BearSSL::WiFiClientSecure> makeSecureClient() {
  std::unique_ptr<BearSSL::WiFiClientSecure> c(new BearSSL::WiFiClientSecure());
  c->setInsecure();                // for production: use setFingerprint(...) or setTrustAnchors(...)
  return c;
}

bool httpPUT_JSON(const char* url, const String& json, String* respOut = nullptr) {
  HTTPClient http;
  auto client = makeSecureClient();
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(*client, url)) return false;
  http.addHeader("Content-Type", "application/json");
  int code = http.sendRequest("PUT", (uint8_t*)json.c_str(), json.length());
  String body = http.getString();
  http.end();
  if (respOut) *respOut = body;
  Serial.printf("[PUT] code=%d\n", code);
  if (body.length()) Serial.println("[PUT] body: " + body);
  return (code >= 200 && code < 300);
}

bool httpGET(const char* url, String& bodyOut) {
  HTTPClient http;
  auto client = makeSecureClient();
  http.setTimeout(15000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  if (!http.begin(*client, url)) return false;
  int code = http.GET();
  bodyOut = http.getString();
  http.end();
  Serial.printf("[GET] code=%d\n", code);
  return (code == 200);
}

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  if (COMMON_ANODE) { r = 255 - r; g = 255 - g; b = 255 - b; }
  analogWrite(RGB_R_PIN, r);
  analogWrite(RGB_G_PIN, g);
  analogWrite(RGB_B_PIN, b);
}

void applyStateJSON(const String& json){
  DynamicJsonDocument doc(1024);              // <-- allocate capacity here
  DeserializationError err = deserializeJson(doc, json);
  if (err) { Serial.println("[JSON] parse error"); return; }

  JsonObject n1 = doc["nodes"]["Node1"];
  const char* led = n1["LED1"] | "off";

  JsonObject rgb = n1["RGB"];
  uint8_t r = rgb["r"] | 0;
  uint8_t g = rgb["g"] | 0;
  uint8_t b = rgb["b"] | 0;

  digitalWrite(LED1_PIN, (strcmp(led,"on")==0) ? HIGH : LOW);
  setRGB(r,g,b);
  Serial.printf("[APPLY] LED1:%s  RGB:(%u,%u,%u)\n", led, r,g,b);
}

// ---------- SETUP / LOOP ----------
void setup() {
  pinMode(LED1_PIN, OUTPUT); digitalWrite(LED1_PIN, LOW);
  pinMode(RGB_R_PIN, OUTPUT); pinMode(RGB_G_PIN, OUTPUT); pinMode(RGB_B_PIN, OUTPUT);
  pinMode(BTN_SEND, INPUT_PULLUP);
  pinMode(BTN_POLL, INPUT_PULLUP);

  Serial.begin(115200);
  Serial.print("WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(300); Serial.print("."); }
  Serial.println(" connected");
}

void tryFlushIFTTTBuffer() {
  if (pendingIFTTT.isEmpty()) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (httpPUT_JSON(URL_IFTTT, pendingIFTTT)) {
    Serial.println("[BUF] pending IFTTT delivered");
    pendingIFTTT = "";
  }
}

void loop() {
  bool doSend = justPressed(BTN_SEND, btnSend);
  bool doPoll = justPressed(BTN_POLL, btnPoll);

  // BTN1: IFTTT
  if (doSend) {
    String payload = "{\"node\":\"Node1\",\"event\":\"switch_pressed\"}";
    if (!httpPUT_JSON(URL_IFTTT, payload)) {
      Serial.println("[IFTTT] send failed, buffering");
      pendingIFTTT = payload;       // keep at most one pending message
    } else {
      Serial.println("[IFTTT] sent OK");
    }
  }

  // Try to flush any buffered message
  tryFlushIFTTTBuffer();

  // BTN2: poll state
  if (doPoll) {
    String body;
    if (httpGET(URL_READ, body)) {
      applyStateJSON(body);
      Serial.println("[POLL] OK");
    } else {
      Serial.println("[POLL] FAIL");
    }
  }
}
