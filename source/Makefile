
DEVICE ?= 0
DEVICE_BASE := /dev/ttyACM
DEVICE_BASE := /dev/ttyUSB
DEVICE_PATH := $(DEVICE_BASE)$(DEVICE)
DEFAULT_ENV := cyd-2432s028r

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

cyd-2432s028r-usb-up:
	$(MAKE) upload ENV=$@

cyd-fnk0103f-usb-up:
	$(MAKE) upload ENV=$@

cyd-fnk0103b-usb-up:
	$(MAKE) upload ENV=$@
	
