#include "application.h"
#include "Thermistor.h"
#include "Soda.h"

// TODO: Use EEPROM.put/get for targetTemp

Soda Soda;

int thermPin = A4; // Analog pin the thermistor is connected to
int thermRes = 4700; // Voltage divider resistor value

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
int upButtonState = 0;
int downButtonState = 0;
int targetTempLastChanged = 0;
int ledBrightnessDirection = 1;
int ledBrightness = 0;

// Exposed variables
bool fanState = false;
bool coolState = false;
int coolLastOn = 0;
int temperature = 0;
int targetTemperature = 72;
String info;

// Timers
Timer displayTimer(1000, displayTemperature);
Timer fanSwitchReadTimer(10, fanSwitchRead);
Timer upButtonReadTimer(10, upButtonRead);
Timer downButtonReadTimer(10, downButtonRead);
Timer toggleCoolTimer(1000, toggleCool);
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
  Particle.variable("fanState", fanState);
  Particle.variable("coolLastOn", coolLastOn);
  Particle.variable("coolState", coolState);
  Particle.variable("temperature", temperature);
  Particle.variable("targetTemp", targetTemperature);
  Particle.function("targetTemp", setTargetTemp);

  //        a   b   c   d   e   f   g   .
  Soda.pins(D3, D2, A3, A5, A6, D5, D6, A2);

  Thermistor.begin();
  displayTimer.start();
  fanSwitchReadTimer.start();
  upButtonReadTimer.start();
  downButtonReadTimer.start();
  toggleCoolTimer.start();
  pulsateLedTimer.start();
  updateInfoTimer.start();

  RGB.control(true);
}

void loop() {
}

void updateInfo() {
  info = String::format(
    "{\"fanState\":\"%s\",\"coolState\":\"%s\",\"coolLastOn\":%d,\"temperature\":%d,\"targetTemperature\":%d}",
    (fanState ? "true" : "false"),
    (coolState ? "true" : "false"),
    coolLastOn,
    temperature,
    targetTemperature
  );
/* coolState=false; */
/* coolLastOn=0; */
/* temperature=0; */
/* targetTemperature=72; */
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
  temperature = round(Thermistor.getTempF(true));
  if ((Time.now() - targetTempLastChanged) >= 6) {
    Soda.write((int) round(temperature) % 10);
    Soda.write(' ');
  }
}

int setTargetTemp(String temp) {
  targetTempLastChanged = Time.now();
  targetTemperature = temp.toInt();
  Soda.write((int) round(targetTemperature) % 10);
  Soda.write('.');
  return targetTemperature;
}

void fanSwitchRead() {
  fanState = (bool) digitalRead(fanSwitch);
  if(!coolState) {
    digitalWrite(fanRelay, fanState);
  }
}

void upButtonRead() {
  upButtonState = digitalRead(upButton);

  if (upButtonState == HIGH) {
    setTargetTemp(String(targetTemperature + 1));
    delay(500);
  }
}

void downButtonRead() {
  downButtonState = digitalRead(downButton);

  if (downButtonState == HIGH) {
    setTargetTemp(String(targetTemperature - 1));
    delay(500);
  }
}

void toggleCool() {
  if (targetTemperature <= temperature) {
    if (Time.now() - coolLastOn > 18) { // TODO: FIXME to 180
      RGB.color(0,0,255);
      coolLastOn = Time.now();
      coolState = true;
      digitalWrite(fanRelay, coolState);
      digitalWrite(coolRelay, coolState);
    } else {
      if (!coolState) {
        RGB.color(255,255,0);
      }
    }
  } else {
    if (coolState) {
      RGB.color(0,255,0);
      coolLastOn = Time.now();
      coolState = false;
      if (!digitalRead(fanSwitch)) {
        digitalWrite(fanRelay, coolState);
      }
      digitalWrite(coolRelay, coolState);
    }
  }
}
