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
int fanRelay = D7; //  D4;
int coolRelay = A1;
int heatRelay = A7;

// States
int upButtonState = 0;
int downButtonState = 0;
int buttonLastPressed = 0;

// Exposed variables
double temperature = 0.0;
int targetTemperature = 72;
int fanState = 0;

// Timers
Timer displayTimer(1000, displayTemperature);
Timer fanSwitchReadTimer(10, fanSwitchRead);
Timer upButtonReadTimer(10, upButtonRead);
Timer downButtonReadTimer(10, downButtonRead);

void setup() {
  pinMode(fanSwitch, INPUT_PULLDOWN);
  pinMode(upButton, INPUT_PULLDOWN);
  pinMode(downButton, INPUT_PULLDOWN);

  pinMode(fanRelay, OUTPUT);
  pinMode(coolRelay, OUTPUT);
  pinMode(heatRelay, OUTPUT);

  Particle.variable("fanState", fanState);
  Particle.variable("temperature", &temperature, DOUBLE);
  Particle.variable("targetTemp", targetTemperature);
  Particle.function("targetTemp", setTargetTemp);

  //        a   b   c   d   e   f   g   .
  Soda.pins(D3, D2, A3, A5, A6, D5, D6, A2);

  Thermistor.begin();
  displayTimer.start();
  fanSwitchReadTimer.start();
  upButtonReadTimer.start();
  downButtonReadTimer.start();
}

void loop() {
}

void displayTemperature() {
  temperature = Thermistor.getTempF(true);
  if ((Time.now() - buttonLastPressed) >= 5) {
    Soda.write((int) round(temperature) % 10);
    Soda.write(' ');
  }
}

int setTargetTemp(String temp) {
  return targetTemperature = temp.toInt();
}

void fanSwitchRead() {
  fanState = digitalRead(fanSwitch);
  digitalWrite(fanRelay, fanState);
}

void upButtonRead() {
  upButtonState = digitalRead(upButton);

  if (upButtonState == HIGH) {
    buttonLastPressed = Time.now();
    targetTemperature = targetTemperature + 1;
    Soda.write((int) round(targetTemperature) % 10);
    Soda.write('.');
    delay(500);
  }
}

void downButtonRead() {

  downButtonState = digitalRead(downButton);

  if (downButtonState == HIGH) {
    buttonLastPressed = Time.now();
    targetTemperature = targetTemperature - 1;
    Soda.write((int) round(targetTemperature) % 10);
    Soda.write('.');
    delay(500);
  }
}
