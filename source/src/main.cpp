/*
 * Galagino - Galaga arcade for ESP32 and Platformio
 *
 * (c) 2025 speckhoiler
 *
 * This is a port of Till Harbaum's awesome Galaga emulator
 * https://github.com/harbaum/galagino
 *
 * Published under GPLv3
 *
 */
#include <Arduino.h>
#include <Esp.h>
#include <esp_flash.h>
#include <rom/spi_flash.h>
#include "config.h"
#include "machines.h"
#include "machines/machineBase.h"
#include "emulation/audio.h"
#include "emulation/video.h"
#include "emulation/input.h"
#include "emulation/menu.h"
#include "emulation/emulation.h"
#ifdef LED_PIN
  #include "emulation/led.h"
#endif

signed char machinesCount = (signed char)(sizeof(machines) / sizeof(unsigned short*));

machineBase *currentMachine;

// the hardware supports 64 sprites
struct sprite_S *sprite_buffer;

// buffer space for one row of 28 characters
unsigned short *frame_buffer;

// RAM
unsigned char *memory;

Audio audio = Audio();
Video video = Video();
Input input = Input();
Menu menu = Menu();
#ifdef LED_PIN
  Led led = Led();
#endif

void updateAudioVideo(void);
void renderRow(short row, bool isMenu);
void onDoAttractReset();
void onVolumeUpDown(bool up, bool down);
void onDoReset();
bool doReset = false;

// defined in Esp.cpp
uint32_t ESP_getFlashChipId(void);

void setup() {
  #if CONFIG_IDF_TARGET_ESP32S3
  delay(2000); // USB delay
  #endif

  Serial.begin(115200);
  delay(200); // let serial initialize
  printf("Galagino\n");

  printf("ESP-IDF:     %s\n", ESP.getSdkVersion());
  printf("Arduino:     %d.%d.%d\n", ESP_ARDUINO_VERSION_MAJOR, ESP_ARDUINO_VERSION_MINOR, ESP_ARDUINO_VERSION_PATCH);
  printf("ESP Chip:    %s - %d\n", ESP.getChipModel(), ESP.getChipRevision());
  printf("CPU Clock:   %d MHz\n", ESP.getCpuFreqMHz());
  printf("Flash Size:  %d MiB\n", ESP.getFlashChipSize() / 1024 / 1024);
  printf("Flash Speed: %d MHz\n", ESP.getFlashChipSpeed() / 1000 / 1000);
  printf("Flash Mode:  %d - ", ESP.getFlashChipMode());
  switch (ESP.getFlashChipMode()) {
    case FM_QIO:       printf("FM_QIO - Quad IO\n"); break;
    case FM_QOUT:      printf("FM_QOUT - Quad Out\n"); break;
    case FM_DIO:       printf("FM_DIO - Dual IO\n"); break;
    case FM_DOUT:      printf("FM_DOUT - Dual Out\n"); break;
    case FM_FAST_READ: printf("FM_FAST_READ\n"); break;
    case FM_SLOW_READ: printf("FM_SLOW_READ\n"); break;
    default:           printf("FM_UNKNOWN\n"); break;
  }
  printf("Flash Manufacturer: 0x%02x\n", (g_rom_flashchip.device_id >> 16) & 0xff);
  printf("Flash Type Code:    0x%02x\n", (g_rom_flashchip.device_id >> 8) & 0xff);
  printf("Flash Size Code:    0x%02x\n", (g_rom_flashchip.device_id >> 0) & 0xff);
  printf("Flash Id:           0x%08x\n", ESP_getFlashChipId());

#ifdef WORKAROUND_I2S_APLL_PROBLEM
  printf("I2S APLL workaround active\n");
#endif

  printf("Free heap: %d\n", ESP.getFreeHeap());
  printf("Main core: %d\n", xPortGetCoreID());
  printf("Main priority: %d\n", uxTaskPriorityGet(NULL));


  #ifdef CONFIG_IDF_TARGET_ESP32
  printf("CONFIG_IDF_TARGET_ESP32:   defined\n");
  #endif
  #ifdef CONFIG_IDF_TARGET_ESP32S2
  printf("CONFIG_IDF_TARGET_ESP32S2: defined\n");
  #endif
  #ifdef CONFIG_IDF_TARGET_ESP32S3
  printf("CONFIG_IDF_TARGET_ESP32S3: defined\n");
  #endif
  printf("TFT_SPI_HOST:  %d\n", TFT_SPI_HOST);

  switch (TFT_SPI_HOST) {
    case SPI1_HOST: printf("TFT_SPI_HOST:  %s\n", "SPI1_HOST"); break;
    case SPI2_HOST: printf("TFT_SPI_HOST:  %s\n", "SPI2_HOST"); break; // esp32 HSPI | esp32-s3 FSPI
    case SPI3_HOST: printf("TFT_SPI_HOST:  %s\n", "SPI3_HOST"); break; // esp32 VSPI | esp32-s3 HSPI
    default:        printf("TFT_SPI_HOST:  %s\n", "?");         break;
  }
  printf("TFT Controller: ");
  #ifdef TFT_ILI9341
  printf("ILI9341\n");
  #else
  printf("ST7789\n");
  #endif
  printf("TFT SPI Clock:  %d MHz\n", TFT_SPICLK/1000000);
  printf("TFT_CS:     %d\n", TFT_CS);
  printf("TFT_DC:     %d\n", TFT_DC);
  printf("TFT_BL:     %d\n", TFT_BL);
  printf("TFT_MISO:   %d\n", TFT_MISO);
  printf("TFT_MOSI:   %d\n", TFT_MOSI);
  printf("TFT_SCLK:   %d\n", TFT_SCLK);
  printf("TFT_RST:    %d\n", TFT_RST);
  printf("TFT_VFLIP:  %s\n",
  #ifdef TFT_VFLIP
  "ON"
  #else
  "OFF"
  #endif
  );
  printf("TFT_INVERT: %s\n", 
  #ifdef TFT_INVERT
  "ON"
  #else
  "OFF"
  #endif
  );
  #ifdef AUDIO_ENABLE_PIN
  printf("AUDIO_ENABLE_PIN: %d\n", AUDIO_ENABLE_PIN);
  #endif

  #ifdef AUDIO_ENABLE_PIN
  pinMode(AUDIO_ENABLE_PIN, OUTPUT);   // ESP32-E Audio Enable
  digitalWrite(AUDIO_ENABLE_PIN, LOW); // active low
  printf("AUDIO_ENABLE:     %s\n", "LOW");
  #endif

  // allocate memory for a single tile/character row
  frame_buffer = (unsigned short*)malloc(224 * 8 * 2);
  sprite_buffer = (sprite_S*)malloc(128 * sizeof(sprite_S));
  memory = (uint8_t *)malloc(RAMSIZE);
  currentMachine = machines[0];

  printf("Before init - Heap: Free=%d MaxAlloc=%d MinFree=%d\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap(), ESP.getMinFreeHeap());
  for (int i = 0; i < machinesCount; i++)
    machines[i]->init(&input, frame_buffer, sprite_buffer, memory);
  printf("After  init - Heap: Free=%d MaxAlloc=%d MinFree=%d\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap(), ESP.getMinFreeHeap());

  audio.init();
  audio.start(currentMachine);

  input.init(machinesCount == 1);
  input.onVolumeUpDown(onVolumeUpDown);
  input.onDoReset(onDoReset);
  input.onDoAttractReset(onDoAttractReset);

  menu.init(&input, machines, machinesCount, frame_buffer);
#ifdef LED_PIN
  led.init();
#endif

  video.begin();
  printf("setup() Heap: Free=%d MaxAlloc=%d MinFree=%d\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap(), ESP.getMinFreeHeap());
}

void loop(void) {
  // run video in main task. This will send signals to the emulation task in the background to synchronize video
  updateAudioVideo();

#ifdef LED_PIN
  led.update(machines, menu.machineIndexPreselection(), menu.machineIndexSelected());
#endif
}

void updateAudioVideo(void) {
  uint32_t t0 = micros();

  bool isMenu = menu.machineIndexIsMenu();
  if(isMenu) {
    menu.handle();
  }
  else {
    if (menu.startMachine()) {
      currentMachine = machines[menu.machineIndexSelected()];
      audio.start(currentMachine);
      video.flip(currentMachine->videoFlipY(), currentMachine->videoFlipX());

      // start new machine
      emulation_start();
    }
    currentMachine->prepare_frame();
  }

  if (doReset || menu.attract_gameTimeout()) {
    // stop current machine
    emulation_stop();
    video.flipReset(currentMachine->videoFlipY(), currentMachine->videoFlipX());

    menu.show_menu();
    doReset = false;
  }

  bool videoHalfRate = true;
#ifndef VIDEO_HALF_RATE
  videoHalfRate = currentMachine->useVideoHalfRate() && !isMenu;
#endif

  if (!videoHalfRate) {
    // render and transmit screen at once as the display running at 80Mhz can update at full 60 hz game frame
    for(int c = 0; c < 36; c += 6) {
      for (int i = 0; i < 6; i++) {
        renderRow(c + i, isMenu); video.write(frame_buffer, 224 * 8);
      }

      // audio is updated 6 times per 60 Hz frame
      audio.transmit();
    }

    emulation_videoRendered();

    // one screen at 60 Hz is 16.6ms
    unsigned long t1 = (micros() - t0) / 1000;  // calculate time in milliseconds
    if(t1 < 16)
      vTaskDelay(16 - t1);
    else
      vTaskDelay(1);    // at least 1 ms delay to prevent watchdog timeout

    // physical refresh is 60Hz. So send vblank trigger once a frame
    emulation_notifyGive();
  }
  else {
    // render and transmit screen in two halfs as the display running at 40Mhz can only update every second 60 hz game frame
    for(int half = 0; half < 2; half++) {
      for(int c = 18 * half; c < 18 * (half + 1); c += 3) {
        renderRow(c + 0, isMenu); video.write(frame_buffer, 224 * 8);
        renderRow(c + 1, isMenu); video.write(frame_buffer, 224 * 8);
        renderRow(c + 2, isMenu); video.write(frame_buffer, 224 * 8);

        // audio is refilled 6 times per screen update. The screen is updated
        // every second frame. So audio is refilled 12 times per 30 Hz frame.
        // Audio registers are udated by CPU3 two times per 30hz frame.
        audio.transmit();
      }

      emulation_videoRendered();

      // one screen at 60 Hz is 16.6ms
      unsigned long t1 = (micros() - t0) / 1000;  // calculate time in milliseconds
      if(t1 < (half ? 33 : 16))
        vTaskDelay((half ? 33 : 16) - t1);
      else if(half)
        vTaskDelay(1);    // at least 1 ms delay to prevent watchdog timeout

      // physical refresh is 30Hz. So send vblank trigger twice a frame to the emulation. This will make the game run with 60hz speed
      emulation_notifyGive();
    }
  }
}

// render one of 36 tile rows (8 x 224 pixel lines)
void renderRow(short row, bool isMenu) {
  if(isMenu) {
    menu.render_row(row);
  }
  else {
    memset(frame_buffer, 0, 2 * 224 * 8);
    currentMachine->render_row(row);
  }
}

void onVolumeUpDown(bool up, bool down) {
  audio.volumeUpDown(up, down);
}

void onDoAttractReset() {
  menu.attract_resetTimer();
}

void onDoReset() {
  if(!menu.machineIndexIsMenu()) {
    doReset = true;
  }
}
