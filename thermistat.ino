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
int rawTemperature = 0;
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

  // Transitioning from high to low will turn the relay on; start high to keep them off
  digitalWrite(fanRelay, true);
  digitalWrite(coolRelay, true);
  digitalWrite(heatRelay, true);

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
    "{ \"mode\": \"%s\", \"heatWait\": %d, \"coolWait\": %d, \"fanState\": %s, \"coolState\": %s, \"heatState\": %s, \"heatLastOn\": %d, \"coolLastOn\": %d, \"rawTemperature\": %d, \"temperature\": %d, \"targetTemperature\": %d }",
    (mode == OFF ? "off" : (mode == COOL ? "cool" : "heat")),
    (targetTemperature > temperature && Time.now() - heatLastOn < 60 ? 60 - (Time.now() - heatLastOn) : 0),
    (targetTemperature < temperature && Time.now() - coolLastOn < 300 ? 300 - (Time.now() - coolLastOn) : 0),
    (fanState ? "true" : "false"),
    (coolState ? "true" : "false"),
    (heatState ? "true" : "false"),
    heatLastOn,
    coolLastOn,
    rawTemperature,
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
  rawTemperature = Thermistor.getTempRaw(true);
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
    if (heatState)
      stopHeat();
    if (coolState)
      stopCool();
    stopCool();
    RGB.color(100,100,100);
  }

  if (requestedMode == "cool") {
    mode = COOL;
    toggleHeatTimer.stop();
    toggleCoolTimer.start();
    if (heatState)
      stopHeat();
  }

  if (requestedMode == "heat") {
    mode = HEAT;
    toggleCoolTimer.stop();
    toggleHeatTimer.start();
    if (coolState)
      stopCool();
  }

  EEPROM.put(20, mode);
  return 0;
}

int setTargetTemp(String temp) {
  int requestedTemp = temp.toInt();
  if (requestedTemp < 60 || requestedTemp > 80)  {
    return 1;
  }
  targetTemperature = requestedTemp;
  targetTempLastChanged = Time.now();
  EEPROM.put(10, targetTempLastChanged);
  EEPROM.put(30, targetTemperature);
  Soda.write((int) round(targetTemperature) % 10);
  Soda.write('.');
  return targetTemperature;
}

void fanSwitchRead() {
  fanState = (bool) digitalRead(fanSwitch);
  if(!coolState && !heatState) {
    digitalWrite(fanRelay, !fanState);
  }
}

void upButtonRead() {
  if (digitalRead(upButton) == HIGH) {
    Soda.write((int) round(targetTemperature) % 10);
    Soda.write('.');

    delay(250);

    if ((Time.now() - targetTempLastChanged) >= 6){
      targetTempLastChanged = Time.now();
      return;
    }

    setTargetTemp(String(targetTemperature + 1));
  }
}

void downButtonRead() {
  if (digitalRead(downButton) == HIGH) {

    Soda.write((int) round(targetTemperature) % 10);
    Soda.write('.');

    delay(250);

    if ((Time.now() - targetTempLastChanged) >= 6){
      targetTempLastChanged = Time.now();
      return;
    }

    setTargetTemp(String(targetTemperature - 1));
  }
}

void toggleCool() {
  if (targetTemperature < temperature) {
    if (Time.now() - coolLastOn > 300) {
      RGB.color(0,0,255);
      coolLastOn = Time.now();
      EEPROM.put(10, coolLastOn);
      coolState = true;
      digitalWrite(fanRelay, !coolState);
      digitalWrite(coolRelay, !coolState);
      Particle.publish("thermostat-on");
    } else {
      if (!coolState) {
        RGB.color(255,255,0);
      }
    }
  }
  if (targetTemperature > temperature && coolState &&
      (Time.now() - coolLastOn > 300 || Time.now() - targetTempLastChanged < 30))
    stopCool();
}

void stopCool() {
  RGB.color(0,255,0);
  coolLastOn = Time.now();
  EEPROM.put(10, coolLastOn);
  coolState = false;
  if (!digitalRead(fanSwitch)) {
    digitalWrite(fanRelay, !coolState);
  }
  digitalWrite(coolRelay, !coolState);
  Particle.publish("thermostat-off");
}

void toggleHeat() {
  if (targetTemperature > temperature) {
    if (Time.now() - heatLastOn > 60) {
      RGB.color(255,0,0);
      heatLastOn = Time.now();
      EEPROM.put(15, heatLastOn);
      heatState = true;
      digitalWrite(fanRelay, !heatState);
      digitalWrite(heatRelay, !heatState);
      Particle.publish("thermostat-on");
    } else {
      if (!heatState) {
        RGB.color(255,255,0);
      }
    }
  }
  if (targetTemperature < temperature && heatState &&
       (Time.now() - heatLastOn > 60 || Time.now() - targetTempLastChanged < 30))
    stopHeat();
}

void stopHeat() {
  RGB.color(0,255,0);
  heatLastOn = Time.now();
  EEPROM.put(15, heatLastOn);
  heatState = false;
  if (!digitalRead(fanSwitch)) {
    digitalWrite(fanRelay, !heatState);
  }
  digitalWrite(heatRelay, !heatState);
  Particle.publish("thermostat-off");
}
