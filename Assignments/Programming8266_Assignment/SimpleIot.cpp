/*
;--------------------------------------
;Title: Sesnsor Database Implementation File
;--------------------------------------
; Program Details:
;--------------------------------------
; Purpose: This program contains the implementation of the classes for sending sensor data to the database
; Inputs: the inputs of the program are the TRIG, ECHO, A0, BUTTON, and TILT pins
; Outputs: values and serial outputs to be used by the main program
; Date: 10/20/2025 02:42pm
; Compiler: PlatformIO Version 5.2.0
; Author: Juan Ali Ruiz Guzman
; Versions:
;               V1 - First version writing the program
;               V2 - Added Function to aalow timezone selection via serial
;               V3 - Fixed minor bugs with WiFi connection and timezone selection
*/
//--------------------------------------
//File Dependencies: 
//--------------------------------------
#include "SimpleIot.hpp"
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h> // for HTTPS support

// ---------- util ----------
String urlEncodeSpaces(const String& s){ String o=s; o.replace(" ","%20"); return o; }

// ---------- TimeClient ----------
TimeClient::TimeClient(String tz) : _tz(std::move(tz)) {}
void TimeClient::setTimezone(const String& tz) { _tz = tz; }

String TimeClient::now() {
  if (WiFi.status() != WL_CONNECTED) return "";

  // HTTPS client for timeapi.io
  std::unique_ptr<BearSSL::WiFiClientSecure> sclient(new BearSSL::WiFiClientSecure);
  sclient->setInsecure();  // accept any cert

  HTTPClient http;
  // Follow redirects in case the service bounces between hosts
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(9000);
  http.setReuse(false);

  // Use HTTPS and the non-www host to avoid extra redirects
  String url = "https://timeapi.io/api/Time/current/zone?timeZone=" + _tz;

  if (!http.begin(*sclient, url)) {
    http.end();
    return "";
  }

  int code = http.GET();
  if (code != 200) {
    // small backoff + one retry helps on flaky links
    http.end();
    delay(400);
    if (!http.begin(*sclient, url)) return "";
    code = http.GET();
    if (code != 200) {
      Serial.printf("Time API HTTP error %d\n", code);
      String _ = http.getString(); // consume body
      http.end();
      return "";
    }
  }

  String body = http.getString();
  http.end();

  // Parse: "dateTime":"YYYY-MM-DDTHH:MM:SS..."
  int p = body.indexOf("\"dateTime\":\"");
  if (p < 0) return "";
  p += 12;
  int q = body.indexOf("\"", p);
  if (q < 0) return "";
  String iso = body.substring(p, q);
  if (iso.length() < 19) return "";

  // "YYYY-MM-DD HH:MM:SS"
  return iso.substring(0,10) + " " + iso.substring(11,19);
}

// ---------- Sensors ----------
UltrasonicSensor::UltrasonicSensor(int trig,int echo,unsigned long timeoutUs)
: _trig(trig),_echo(echo),_timeoutUs(timeoutUs){ pinMode(_trig,OUTPUT); pinMode(_echo,INPUT); digitalWrite(_trig,LOW); }
float UltrasonicSensor::read(){
  digitalWrite(_trig,LOW); delayMicroseconds(2);
  digitalWrite(_trig,HIGH); delayMicroseconds(10);
  digitalWrite(_trig,LOW);
  unsigned long us=pulseIn(_echo,HIGH,_timeoutUs);
  if(us==0) return NAN;
  return us/58.0f; // cm
}
PhotoSensor::PhotoSensor(int adcPin):_adcPin(adcPin){}
float PhotoSensor::read(){ return (float)analogRead(_adcPin); }

// ---------- Triggers ----------
DebouncedInput::DebouncedInput(int pin,bool activeLow):_pin(pin),_activeLow(activeLow){
  pinMode(_pin, INPUT);  // using external resistors
  _stable = _lastRaw = (_activeLow? digitalRead(_pin)==LOW : digitalRead(_pin)==HIGH);
}
bool DebouncedInput::fired(){
  bool raw = (_activeLow? digitalRead(_pin)==LOW : digitalRead(_pin)==HIGH);
  unsigned long now=millis();
  if(raw!=_lastRaw){ _lastRaw=raw; _tMs=now; }
  if(now-_tMs>=_deb){
    if(raw!=_stable){ _stable=raw; if(_stable) return true; }
  }
  return false;
}

// ---------- Transmitter ----------
Transmitter::Transmitter(String endpoint): _endpoint(std::move(endpoint)) {}

bool Transmitter::send(const String& node, const String& ts,
                       float proximity, float light, const String& source) {
  if (WiFi.status() != WL_CONNECTED) return false;
  if (ts.length() < 19) return false;

  String url = _endpoint + "?node_name=" + node + "&time_received=" + urlEncodeSpaces(ts);
  if (!isnan(proximity)) url += "&proximity=" + String(proximity, 2);
  if (!isnan(light))     url += "&light="     + String(light, 0);
  if (source.length())   url += "&source="    + source;

  // HTTPS client
  std::unique_ptr<BearSSL::WiFiClientSecure> sclient(new BearSSL::WiFiClientSecure);
  sclient->setInsecure();  // easiest for class projects

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.setTimeout(9000);
  http.setReuse(false);

  if (!http.begin(*sclient, url)) return false;

  int code = http.GET();
  String body = http.getString();
  http.end();

  Serial.printf("HTTP %d : %s\n", code, body.c_str());
  return code == 200;
}


// ---------- Node ----------
Node::Node(String name,String source,DebouncedInput& trig,Sensor& sensor,bool asProximity)
: _name(std::move(name)),_src(std::move(source)),_trig(trig),_sensor(sensor),_asProx(asProximity){}
bool Node::process(TimeClient& tc, Transmitter& tx){
  if(!_trig.fired()) return false;
  String ts=tc.now(); if(ts.isEmpty()){ Serial.println("Time failed"); return false; }
  float v=_sensor.read(); if(isnan(v)){ Serial.println("Sensor failed"); return false; }
  float prox=NAN, light=NAN; if(_asProx) prox=v; else light=v;
  Serial.printf("[%s] %.2f @ %s\n",_name.c_str(),v,ts.c_str());
  return tx.send(_name,ts,prox,light,_src);
}

// ---------- App ----------
App::App(String tz,String endpoint,int pTrig,int pEcho,int pLDR,int pBtn,int pTilt)
: _time(std::move(tz)), _tx(std::move(endpoint)),
  _ultra(pTrig,pEcho), _photo(pLDR),
  _btn(pBtn,true), _tilt(pTilt,true),
  _node1("Node_1","SWITCH",_btn,_ultra,true),
  _node2("Node_2","TILT",  _tilt,_photo,false) {}

void App::setTimezone(const String& tz){ _time.setTimezone(tz); }
void App::beginWifi(const char* ssid, const char* pass, uint16_t maxWaitMs) {
  // Clean start but don't spam flash with creds
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);          // clear any old state
  delay(150);

  WiFi.begin(ssid, pass);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0 < maxWaitMs)) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi OK, IP: ");
    Serial.println(WiFi.localIP());     // keep one useful line
  } else {
    Serial.println("WiFi FAILED");      // short failure note, no “tips”
  }
}


void App::tick(){ (void)_node1.process(_time,_tx); (void)_node2.process(_time,_tx); }

// ====== Internal TZ option table (Appendix A mapping) ======
struct TzOption { const char* name; const char* iana; };
static const TzOption TZ_OPTS[7] = {
  { "Eastern (ET)   – New York, NY",         "America/New_York"    }, // 1
  { "Central (CT)   – Chicago, IL",          "America/Chicago"     }, // 2
  { "Mountain (MT)  – Denver, CO",           "America/Denver"      }, // 3
  { "Pacific (PT)   – Los Angeles, CA",      "America/Los_Angeles" }, // 4 (default)
  { "Alaska (AKT)   – Anchorage, AK",        "America/Anchorage"   }, // 5
  { "Hawaii (HAT)   – Honolulu, HI",         "Pacific/Honolulu"    }, // 6
  { "Atlantic (AT)  – San Juan, Puerto Rico","America/Puerto_Rico" }  // 7
};

String selectTimezoneOverSerial(uint32_t timeoutMs) {
  Serial.println();
  Serial.println(F("—> Select Your Time Zone (default is 4 = Pacific Time):"));
  for (int i = 0; i < 7; ++i) {
    Serial.printf("  %d) %s\n", i + 1, TZ_OPTS[i].name);
  }
  Serial.println();
  Serial.printf("Enter 1-7 then press Enter (waiting %lu ms)...\n", (unsigned long)timeoutMs);

  String entry;
  unsigned long t0 = millis();
  while (millis() - t0 < timeoutMs) {
    if (Serial.available()) {
      entry = Serial.readStringUntil('\n');
      entry.trim();
      break;
    }
    delay(20);
  }

  int choice = 4; // default = Pacific
  if (entry.length()) {
    int c = entry.toInt();
    if (c >= 1 && c <= 7) choice = c;
  }

  const char* zone = TZ_OPTS[choice - 1].iana;
  Serial.printf("Using timezone: [%d] %s\n\n", choice, zone);
  return String(zone);
}

App* createAppWithTimezoneAndWifi(
    const char* ssid,
    const char* pass,
    const String& endpoint,
    int pinTrig, int pinEcho, int pinLdr, int pinBtn, int pinTilt,
    uint32_t tzTimeoutMs
) {
  // 1) Pick timezone via USB
  String tz = selectTimezoneOverSerial(tzTimeoutMs);

  // 2) Build the app with that timezone and your pins
  App* app = new App(tz, endpoint, pinTrig, pinEcho, pinLdr, pinBtn, pinTilt);

  // 3) Connect Wi-Fi (uses App::beginWifi you already have)
  app->beginWifi(ssid, pass);

  return app;
}

