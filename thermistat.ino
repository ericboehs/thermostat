#include "application.h"
#include "Thermistor.h"
#include "Soda.h"

#define OFF 0
#define COOL 1
#define HEAT 2

Soda Soda;

int thermPin = A4; // Analog pin the thermistor is connected to
int thermRes = 4000; // Voltage divider resistor value

// Configure the Thermistor class
Thermistor Thermistor(thermPin, thermRes);

// Inputs
int fanSwitch = D1;
int upButton = D0;
int downButton = A0;

// Outputs
int fanRelay = D4;
int coolRelay = A1;
int heatRelay = A7;

// States
int targetTempLastChanged = 0;
int ledBrightnessDirection = 1;
int ledBrightness = 0;

// Exposed variables
bool fanState = false;
bool heatState = false;
bool coolState = false;
int mode = COOL; // OFF, COOL, HEAT
int heatLastOn = 0;
int coolLastOn = 0;
int temperature = 0;
int targetTemperature = 72;
double preciseTemperature = 0.0;
String info;

// Timers
Timer displayTimer(1000, displayTemperature);
Timer fanSwitchReadTimer(10, fanSwitchRead);
Timer upButtonReadTimer(10, upButtonRead);
Timer downButtonReadTimer(10, downButtonRead);
Timer toggleCoolTimer(1000, toggleCool);
Timer toggleHeatTimer(1000, toggleHeat);
Timer pulsateLedTimer(25, pulsateLed);
Timer updateInfoTimer(100, updateInfo);

void setup() {
  pinMode(fanSwitch, INPUT_PULLDOWN);
  pinMode(upButton, INPUT_PULLDOWN);
  pinMode(downButton, INPUT_PULLDOWN);

  pinMode(fanRelay, OUTPUT);
  pinMode(coolRelay, OUTPUT);
  pinMode(heatRelay, OUTPUT);

  Particle.variable("info", &info, STRING);
  Particle.variable("mode", mode);
  Particle.variable("fanState", fanState);
  Particle.variable("heatLastOn", heatLastOn);
  Particle.variable("heatState", heatState);
  Particle.variable("coolLastOn", coolLastOn);
  Particle.variable("coolState", coolState);
  Particle.variable("temperature", temperature);
  Particle.variable("targetTemp", targetTemperature);
  Particle.function("setMode", setMode);
  Particle.function("targetTemp", setTargetTemp);

  EEPROM.get(10, coolLastOn);
  EEPROM.get(15, heatLastOn);
  EEPROM.get(20, mode);
  EEPROM.get(30, targetTemperature);
  if (coolLastOn == 0xFFFF) coolLastOn = Time.now();
  if (heatLastOn == 0xFFFF) heatLastOn = Time.now();
  if (mode == 0xFFFF) mode = OFF;
  if (targetTemperature == 0xFFFF) targetTemperature = 72;

  RGB.control(true);
  RGB.color(0, 255, 0);

  //        a   b   c   d   e   f   g   .
  Soda.pins(D3, D2, A3, A5, A6, D5, D6, A2);

  Thermistor.begin();
  displayTimer.start();
  fanSwitchReadTimer.start();
  upButtonReadTimer.start();
  downButtonReadTimer.start();
  if (mode == COOL) toggleCoolTimer.start();
  if (mode == HEAT) toggleHeatTimer.start();
  if (mode == OFF) RGB.color(100, 100, 100);
  pulsateLedTimer.start();
  updateInfoTimer.start();

}

void loop() {
}

void updateInfo() {
  info = String::format(
    "{ \"mode\": \"%s\", \"fanState\": %s, \"coolState\": %s, \"heatState\": %s, \"heatLastOn\": %d, \"coolLastOn\": %d, \"temperature\": %d, \"targetTemperature\": %d }",
    (mode == OFF ? "off" : (mode == COOL ? "cool" : "heat")),
    (fanState ? "true" : "false"),
    (coolState ? "true" : "false"),
    (heatState ? "true" : "false"),
    heatLastOn,
    coolLastOn,
    temperature,
    targetTemperature
  );
}

void pulsateLed() {
  if (ledBrightnessDirection) {
    ledBrightness = ledBrightness + 1;
  } else {
    ledBrightness = ledBrightness - 1;
  }

  if (ledBrightness <= 0 ) {
    ledBrightnessDirection = 1;
  }

  if (ledBrightness >= 100 ) {
    ledBrightnessDirection = 0;
  }

  RGB.brightness(ledBrightness);
}

void displayTemperature() {
  preciseTemperature = Thermistor.getTempF(true);
  temperature = round(preciseTemperature);
  if ((Time.now() - targetTempLastChanged) >= 6) {
    Soda.write((int) round(temperature) % 10);
    Soda.write(' ');
  }
}

int setMode(String requestedMode) {
  if (requestedMode == "off") {
    mode = OFF;
    toggleCoolTimer.stop();
    toggleHeatTimer.stop();
    stopHeat();
    stopCool();
    RGB.color(100,100,100);
  }

  if (requestedMode == "cool") {
    mode = COOL;
    toggleHeatTimer.stop();
    toggleCoolTimer.start();
    stopHeat();
  }

  if (requestedMode == "heat") {
    mode = HEAT;
    toggleCoolTimer.stop();
    toggleHeatTimer.start();
    stopCool();
  }

  EEPROM.put(20, mode);
  return 0;
}

int setTargetTemp(String temp) {
  targetTempLastChanged = Time.now();
  targetTemperature = temp.toInt();
  EEPROM.put(10, targetTempLastChanged);
  EEPROM.put(30, targetTemperature);
  Soda.write((int) round(targetTemperature) % 10);
  Soda.write('.');
  return targetTemperature;
}

void fanSwitchRead() {
  fanState = (bool) digitalRead(fanSwitch);
  if(!coolState && !heatState) {
    digitalWrite(fanRelay, fanState);
  }
}

void upButtonRead() {
  if (digitalRead(upButton) == HIGH) {
    setTargetTemp(String(targetTemperature + 1));
    delay(250);
  }
}

void downButtonRead() {
  if (digitalRead(downButton) == HIGH) {
    setTargetTemp(String(targetTemperature - 1));
    delay(250);
  }
}

void toggleCool() {
  if (targetTemperature <= temperature) {
    if (Time.now() - coolLastOn > 18) { // TODO: FIXME to 180
      RGB.color(0,0,255);
      coolLastOn = Time.now();
      EEPROM.put(10, coolLastOn);
      coolState = true;
      digitalWrite(fanRelay, coolState);
      digitalWrite(coolRelay, coolState);
    } else {
      if (!coolState) {
        RGB.color(255,255,0);
      }
    }
  } else if (coolState) stopCool();
}

void stopCool() {
  RGB.color(0,255,0);
  coolLastOn = Time.now();
  EEPROM.put(10, coolLastOn);
  coolState = false;
  if (!digitalRead(fanSwitch)) {
    digitalWrite(fanRelay, coolState);
  }
  digitalWrite(coolRelay, coolState);
}

void toggleHeat() {
  if (targetTemperature >= temperature) {
    if (Time.now() - heatLastOn > 18) { // TODO: FIXME to 180
      RGB.color(255,0,0);
      heatLastOn = Time.now();
      EEPROM.put(15, heatLastOn);
      heatState = true;
      digitalWrite(fanRelay, heatState);
      digitalWrite(heatRelay, heatState);
    } else {
      if (!heatState) {
        RGB.color(255,255,0);
      }
    }
  } else if (heatState) stopHeat();
}

void stopHeat() {
  RGB.color(0,255,0);
  heatLastOn = Time.now();
  EEPROM.put(15, heatLastOn);
  heatState = false;
  if (!digitalRead(fanSwitch)) {
    digitalWrite(fanRelay, heatState);
  }
  digitalWrite(heatRelay, heatState);
}
