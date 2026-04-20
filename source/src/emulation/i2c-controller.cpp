#include "i2c-controller.h"

#ifdef GALAGINO_CONTROLLER
#include <esp32-hal-i2c.h>

// port0 by default
#ifndef GALAGINO_CONTROLLER_USE_I2C_PORT1
  #define WIRE Wire
  #define I2C_PORT 0
#else
  #define WIRE Wire1
  #define I2C_PORT 1
#endif

void ControllerI2C::scan(uint8_t want) {

  int8_t  err   = 0;
  uint8_t addr  = 0;
  uint8_t count = 0;

  printf("Scanning...\n");

  disable();
  dead=true;
  for (addr = 0x08; addr<0x70; addr++) {
    WIRE.beginTransmission(addr);
    err = WIRE.endTransmission();

    if (err == 0) {
      printf("Addr=0x%0x ", addr);
      if (err == 0) printf(" (0) found device");
      printf("\n");
      if (addr == want) {
        dead=false;
        enable();
      }
    }
  }
}

void ControllerI2C::setup() {

  bool isInit = i2cIsInit(I2C_PORT);
  if (isInit) {
    printf("controller: already init Clock=%d\n", WIRE.getClock());
  }
  else {
    bool done = WIRE.begin(GALAGINO_CONTROLLER_SDA, GALAGINO_CONTROLLER_SCL, 400000);
    printf("controller: init=%d SDA=%d SCL=%d Clock=%d\n",
      done, GALAGINO_CONTROLLER_SDA, GALAGINO_CONTROLLER_SCL,
      WIRE.getClock());
  }

  WIRE.beginTransmission((uint8_t)GALAGINO_CONTROLLER_ADDR);
  int8_t err = WIRE.endTransmission();
  printf("Transmission: err=%d\n", err);
  lastUpdate = millis();
  //scan(GALAGINO_CONTROLLER_ADDR);
  enable();
}

void ControllerI2C::enable() {
  WIRE.beginTransmission(GALAGINO_CONTROLLER_ADDR);
  int8_t err = WIRE.endTransmission();

  printf("Addr=0x%0x err=%d\n", GALAGINO_CONTROLLER_ADDR, err);
  dead = (err != 0);
  if (dead) {
    printf("ControllerI2C::enable (dead) \n");
    return;
  }

  enabled = true;
  printf("ControllerI2C::enable\n");
}

void ControllerI2C::disable() {
  enabled = false;
  printf("ControllerI2C::disable\n");
}

#ifndef CONTROLLER_UPDATE_MILLIS
#define CONTROLLER_UPDATE_MILLIS 33
#endif

unsigned char ControllerI2C::getInput() {

  unsigned long now = millis();
  if (!enabled || now - lastUpdate < CONTROLLER_UPDATE_MILLIS) {
    // i2c updated only every 33ms (2 frames)
    return lastValue;
  } else {
    lastUpdate = now;
    uint8_t count =  WIRE.requestFrom((uint8_t)GALAGINO_CONTROLLER_ADDR, (uint8_t)1);
    if (WIRE.available()) lastValue = WIRE.read();
    while (WIRE.available()) {
      WIRE.read();
    }
  }
  return lastValue;
}

#endif
