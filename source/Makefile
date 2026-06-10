
DEVICE_BASE := /dev/ttyUSB
DEVICE_BASE := /dev/ttyACM
DEFAULT_ENV := cyd-2432s028r
DEFAULT_ENV := cyd-esp32-s3-display-usb-up

ifneq (,$(wildcard /dev/ttyUSB*))
  DEVICE_BASE := /dev/ttyUSB
endif
ifneq (,$(wildcard /dev/ttyACM*))
  DEVICE_BASE := /dev/ttyACM
endif
DEVICE ?= 0
DEVICE_PATH := $(DEVICE_BASE)$(DEVICE)

ENV ?= $(DEFAULT_ENV)

ifeq ($(V),)
        VERBOSE :=
else
        VERBOSE := --verbose --jobs=1
endif

.PHONY:
config:
	pio project config

.PHONY: build
build:
	pio run $(VERBOSE) -e $(ENV)

.PHONY: monitor
monitor:
	pio device monitor -e $(ENV) --port $(DEVICE_PATH)

.PHONY:upload
upload:
	pio run $(VERBOSE) -e $(ENV) -t upload -t monitor

.PHONY: upload-only
upload-only:
	pio run $(VERBOSE) -e $(ENV) -t upload

cyd-2432s028r:
	$(MAKE) upload ENV=$@

cyd-fnk0103f:
	$(MAKE) upload ENV=$@

cyd-fnk0103b:
	$(MAKE) upload ENV=$@

# ST7789 - black
cyd-fnk0103b-usb-up:
	$(MAKE) upload ENV=$@

# ILI9341 - transparent
cyd-fnk0103f-usb-up:
	$(MAKE) upload ENV=$@

# ILI9341 - silk white
cyd-esp32-s3-display-usb-up:
	$(MAKE) upload ENV=$@

image-info:
	esptool --chip esp32s3 --port $(DEVICE_PATH) image-info .pio/build/cyd-esp32-s3-display-usb-up/GALAGINO_*_release.bin

flash-id:
	esptool --port $(DEVICE_PATH) flash-id

