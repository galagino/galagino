#ifndef _CONFIG_H_
#define _CONFIG_H_

// game config
#define MASTER_ATTRACT_MENU_TIMEOUT  20000      // start games while sitting idle in menu for 20 seconds, undefine to disable
#define MASTER_ATTRACT_GAME_TIMEOUT  60000 * 5  // restart after 5 minutes 

// video config
//#define TFT_SPICLK  40000000    // 40 Mhz. Some displays cope with 80 Mhz
//#define TFT_SPICLK  80000000    // 80 Mhz. Some displays cope with 80 Mhz

// ILI9341 don't work reliably above 60 Mhz, ST7789 works fine at 80 Mhz

#ifndef TFT_SPICLK
#define TFT_SPICLK  40000000 // safe default in not defined on platformio.ini
#endif

// max possible video rate:
// 8*224 pixels = 8*224*16 = 28672 bits
// 2790 char rows per sec at 40Mhz = max 38 fps
#if TFT_SPICLK < 80000000
  #define VIDEO_HALF_RATE
#endif

// x and y offset of 224x288 pixels inside the 240x320 screen
#define TFT_X_OFFSET      8
#define TFT_Y_OFFSET      16

// led config
//#define LED_PIN           18 // pin used for optional WS2812 stripe
#define LED_BRIGHTNESS 	  50 // range 0..255

// audio config
//#define SND_DIFF   	 // set to output differential audio on GPIO25 _and_ inverted on GPIO26
#define SND_LEFT_CHANNEL // Use GPIO 26 for audio

// esp32 model config
//#define CHEAP_YELLOW_DISPLAY_CONF

#ifndef USE_PIO_CONFIG
#ifdef CHEAP_YELLOW_DISPLAY_CONF
  #define TFT_CS          15
  #define TFT_DC          2
  #define TFT_RST         -1
  #define TFT_BL          27   // don't set if backlight is hard wired
  #define TFT_BL_LEVEL    HIGH  // backlight on with low or high signal
  //#define TFT_ILI9341 // define for ili9341, otherwise st7789
  //#define TFT_VFLIP   // define for upside down

  #define TFT_MISO 	      12
  #define TFT_MOSI 	      13
  #define TFT_SCLK 	      14
  //#define TFT_MAC  	    0x20  // some CYD need this to rotate properly and have correct colors

  // Pins used for buttons
  #define BTN_START_PIN	  35
  //#define BTN_COIN_PIN    21   // if this is not defined, then start will act as coin & start

  #define BTN_LEFT_PIN    21
  #define BTN_RIGHT_PIN   22
  #define BTN_DOWN_PIN    16
  #define BTN_UP_PIN      17
  #define BTN_FIRE_PIN    4
#endif

#ifndef CHEAP_YELLOW_DISPLAY_CONF
  #define TFT_CS          5
  #define TFT_DC          4
  #define TFT_RST         22
  #define TFT_BL          15      // don't set if backlight is hard wired
  #define TFT_BL_LEVEL    LOW     // backlight on with low or high signal
  #define TFT_ILI9341             // define for ili9341, otherwise st7789
  //#define TFT_VFLIP               // define for upside down

  #define TFT_MISO 	      19
  #define TFT_MOSI 	      23
  #define TFT_SCLK 	      18

  // Pins used for buttons
  //#define BTN_START_PIN   0
  //#define BTN_COIN_PIN    21      // if this is not defined, then start will act as coin & start

  #ifndef NUNCHUCK_INPUT
    #define BTN_LEFT_PIN  33
    #define BTN_RIGHT_PIN 14
    #define BTN_DOWN_PIN  16
    #define BTN_UP_PIN    21
    #define BTN_FIRE_PIN  12
  #else
    #define NUNCHUCK_SDA  33
    #define NUNCHUCK_SCL  32
    #define NUNCHUCK_MOVE_THRESHOLD 30 // This is the dead-zone for where minor movements on the stick will not be considered valid movements
  #endif
#endif
#endif

#ifndef TFT_SPI_HOST

#if CONFIG_IDF_TARGET_ESP32S3
// ESP32-S3
// SPI0/SPI1 -> FLASH/PSRAM 
// SPI2      -> 80MHz IO_MUX pins 9/10/11/12/13/14
// SPI3      -> 40Mhz GPIO matrix any pins
#define TFT_SPI_HOST SPI2_HOST
#else
// ESP32
// SPI0/SPI1 -> FLASH/PSRAM 
// SPI2      -> 80MHz 2/4/12=HSPI_MISO/13=HSPI_MOSI/14=HSPI_CLK/15=HSPI_CS0
// SPI3      -> 80Mhz 5=VSPI_CS0/18=VSPI_CLK/19=VSPI_MISO/23=VSPI_MOSI 22=VSPI_WP 21=VSPI_HD
#define TFT_SPI_HOST SPI2_HOST
#endif

#endif

#endif // _CONFIG_H_
