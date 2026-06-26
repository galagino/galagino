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

#define GALAGINO_BUTTON_LEFT  0x01
#define GALAGINO_BUTTON_RIGHT 0x02
#define GALAGINO_BUTTON_UP    0x04
#define GALAGINO_BUTTON_DOWN  0x08
#define GALAGINO_BUTTON_FIRE  0x10
#define GALAGINO_BUTTON_START 0x20
#define GALAGINO_BUTTON_COIN  0x40
#define GALAGINO_BUTTON_EXTRA 0x80

#define GALAGINO_BUTTON_A    0x0100
#define GALAGINO_BUTTON_B    0x0200
#define GALAGINO_BUTTON_X    0x0400
#define GALAGINO_BUTTON_Y    0x0800

#define GALAGINO_BUTTON_MENU 0x8000

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
  unsigned int getInput();

private:
  bool enabled;
  bool dead;
  unsigned long lastUpdate;
  unsigned int lastValue;
};

#endif
