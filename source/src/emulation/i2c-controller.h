#pragma once

#include "../config.h"

#ifdef GALAGINO_CONTROLLER
#include <Arduino.h>
#include <Wire.h>

// a total of 7 button is needed for most games
#define BUTTON_LEFT  0x01
#define BUTTON_RIGHT 0x02
#define BUTTON_UP    0x04
#define BUTTON_DOWN  0x08
#define BUTTON_FIRE  0x10
#define BUTTON_START 0x20
#define BUTTON_COIN  0x40
#define BUTTON_EXTRA 0x80

#ifndef GALAGINO_CONTROLLER_ADDR
#define GALAGINO_CONTROLLER_ADDR 0x09
#endif
#ifndef GALAGINO_CONTROLLER_SDA
#define GALAGINO_CONTROLLER_SDA 27
#endif
#ifndef GALAGINO_CONTROLLER_SDA
#define GALAGINO_CONTROLLER_SCL 22
#endif

class ControllerI2C {
public:
  void setup();
  void enable();
  void disable();
  void scan(uint8_t);
  unsigned char getInput();

private:
  bool enabled;
  bool dead;
  unsigned long lastUpdate;
  unsigned char lastValue;
};

#endif
