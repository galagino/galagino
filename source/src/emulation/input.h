#ifndef INPUT_H
#define INPUT_H

#include <Arduino.h>
#include "../config.h"
#ifdef NUNCHUCK_INPUT
  #include "nunchuck.h"
#endif
#ifdef GALAGINO_CONTROLLER
#include "i2c-controller.h"
#endif

// a total of 7 button is needed for most games
#define BUTTON_LEFT  0x01
#define BUTTON_RIGHT 0x02
#define BUTTON_UP    0x04
#define BUTTON_DOWN  0x08
#define BUTTON_FIRE  0x10
#define BUTTON_START 0x20
#define BUTTON_COIN  0x40
#define BUTTON_EXTRA 0x80

#define BUTTON_A    0x0100
#define BUTTON_B    0x0200
#define BUTTON_X    0x0400
#define BUTTON_Y    0x0800
#define BUTTON_L1   0x1000
#define BUTTON_R1   0x2000

#define BUTTON_MENU 0x8000

class Input {
public:
  void init(bool SingleMachine);
  void enable();
  void disable();
  unsigned int buttons_get(void);
  bool button_y_pressed(void);
  unsigned int fire_raw(void) { return fire_raw_state; }
  bool demoSoundsOff();

  typedef std::function<void(bool up, bool down)> THandlerVolume;
  Input& onVolumeUpDown(THandlerVolume fn);

  typedef std::function<void(void)> THandlerDoReset;
  Input& onDoReset(THandlerDoReset fn);

  typedef std::function<void(void)> THandlerDoAttractReset;
  Input& onDoAttractReset(THandlerDoAttractReset fn);

private:
  THandlerVolume _volume_callback;
  THandlerDoReset _doReset_callback;
  THandlerDoAttractReset _doAttractReset_callback;
  unsigned int input_states_last;
  int virtual_coin_state;
  unsigned long virtual_coin_timer;
  unsigned long reset_timer;
  bool singleMachine;
  bool switchDemoSoundsOff;
  bool firePressedAtStart;
  unsigned int fire_raw_state = 0;
#ifdef NUNCHUCK_INPUT
  Nunchuck nunchuck;
#endif
#ifdef GALAGINO_CONTROLLER
  ControllerI2C Controller;
#endif
};

#endif
