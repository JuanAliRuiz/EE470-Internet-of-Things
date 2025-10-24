/*
;--------------------------------------
;Title: Sensor Database Header File
;--------------------------------------
; Program Details:
;--------------------------------------
; Purpose: This program defines the classes for sending sensor data to the database
; Inputs: N/A
; Outputs: A header file with defined classes to be uses by the implementation file
; Date: 10/20/2025 02:42pm
; Compiler: PlatformIO Version 5.2.0
; Author: Juan Ali Ruiz Guzman
; Versions:
;               V1 - First version writing the program
*/
//--------------------------------------
//File Dependencies: 
//--------------------------------------
#ifndef SIMPLEIOT_HPP
#define SIMPLEIOT_HPP
#include <Arduino.h>

// ---------- Time ----------
class TimeClient {
  String _tz;
public:
  explicit TimeClient(String tz);
  void setTimezone(const String& tz);
  String now(); // "YYYY-MM-DD HH:MM:SS" or ""
};

// ---------- Sensors ----------
class Sensor { public: virtual ~Sensor()=default; virtual float read()=0; };
class UltrasonicSensor : public Sensor {
  int _trig,_echo; unsigned long _timeoutUs;
public:
  UltrasonicSensor(int trig,int echo,unsigned long timeoutUs=30000UL);
  float read() override; // cm (NAN on timeout)
};
class PhotoSensor : public Sensor {
  int _adcPin; // A0
public:
  explicit PhotoSensor(int adcPin);
  float read() override; // 0..1023
};

// ---------- Triggers (debounced, active-low) ----------
class DebouncedInput {
  int _pin; bool _activeLow;
  unsigned long _tMs=0; bool _lastRaw=false, _stable=false; unsigned long _deb=30;
public:
  DebouncedInput(int pin,bool activeLow);
  bool fired(); // rising edge once per press/tilt
};

// ---------- Transmitter ----------
class Transmitter {
  String _endpoint;
public:
  explicit Transmitter(String endpoint);
  bool send(const String& node,const String& ts, float proximity, float light, const String& source);
};

// ---------- Node (one trigger + one sensor) ----------
class Node {
  String _name,_src; DebouncedInput& _trig; Sensor& _sensor; bool _asProx; // true=>proximity, false=>light
public:
  Node(String name,String source,DebouncedInput& trig,Sensor& sensor,bool asProximity);
  bool process(TimeClient& tc, Transmitter& tx);
};

// ---------- App ----------
class App {
  TimeClient _time; Transmitter _tx;
  // hardware
  UltrasonicSensor _ultra;
  PhotoSensor _photo;
  DebouncedInput _btn;
  DebouncedInput _tilt;
  // nodes
  Node _node1; // button -> ultrasonic -> proximity
  Node _node2; // tilt   -> photo      -> light
public:
  App(String tz,String endpoint,int pTrig,int pEcho,int pLDR,int pBtn,int pTilt);
  void setTimezone(const String& tz);
  void beginWifi(const char* ssid,const char* pass,uint16_t maxWaitMs=15000);
  void tick();
};

// ---------- tiny util ----------
String urlEncodeSpaces(const String& s);

// ===== Timezone selection + one-call app bootstrap =====
// Shows the 7-option timezone menu over Serial and returns the chosen IANA zone.
// If the user doesn't enter anything before timeoutMs, defaults to Pacific (PT).
String selectTimezoneOverSerial(uint32_t timeoutMs = 10000);

// Builds an App with the chosen timezone, connects Wi-Fi, and returns it.
// This keeps main.cpp super minimal.
App* createAppWithTimezoneAndWifi(
    const char* ssid,
    const char* pass,
    const String& endpoint,
    int pinTrig, int pinEcho, int pinLdr, int pinBtn, int pinTilt,
    uint32_t tzTimeoutMs = 10000
);

#endif
