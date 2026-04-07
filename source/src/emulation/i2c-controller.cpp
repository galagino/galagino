#include "i2c-controller.h"

#ifdef GALAGINO_CONTROLLER

void ControllerI2C::scan() {

  uint8_t err   = 0;
  uint8_t addr  = 0;
  uint8_t count = 0;

  Serial.printf("Scanning...\n");

  disable();
  dead=true;
  for (addr = 0x08; addr<0x70; addr++) {
    Wire.beginTransmission(addr);
    err = Wire.endTransmission();

    if (err == 0) {
      Serial.printf("Addr=0x%0x ", addr);
      if (err == 0) Serial.printf(" (0) found device");
      Serial.printf("\n");
      dead=false;
      enable();
    }
  }
  delay(500);
}

void ControllerI2C::setup() {

  bool done = Wire.begin(GALAGINO_CONTROLLER_SDA, GALAGINO_CONTROLLER_SCL, 400000);
  Serial.printf("controller: done=%d SDA=%d SCL=%d Clock=%d\n",
    done,
    GALAGINO_CONTROLLER_SDA,
    GALAGINO_CONTROLLER_SCL,
    Wire.getClock());

  Wire.beginTransmission((uint8_t)GALAGINO_CONTROLLER_ADDR);
  uint8_t error = Wire.endTransmission();
  Serial.printf("Transmission: %d\n", error);
  delay(1000);
  lastUpdate = millis();
  enable();
  scan();
}

void ControllerI2C::enable() {
  if (dead) return;
  enabled = true;
  Serial.println("ControllerI2C::enable");
}

void ControllerI2C::disable() {
  enabled = false;
  Serial.println("ControllerI2C::disable");
}

unsigned char ControllerI2C::getInput() {

  unsigned long now = millis();
  if (!enabled || now - lastUpdate < 33) {
    // i2c updated only every 33ms (2 frames)
    return lastValue;
  } else {
    lastUpdate = now;
    uint8_t count =  Wire.requestFrom((uint8_t)GALAGINO_CONTROLLER_ADDR, (uint8_t)1);
    if (Wire.available()) lastValue = Wire.read();
    while (Wire.available()) {
      Wire.read();
    }
  }
  return lastValue;
}

#endif
