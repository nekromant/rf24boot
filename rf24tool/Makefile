CFLAGS  = $(shell pkg-config --cflags libusb-1.0)
LDFLAGS = $(shell pkg-config --libs libusb-1.0)

MAKEFLAGS+=--no-print-directory -r

CROSS_COMPILE?=

all: rf24tool

SRCS= adaptor-factory.c rf24tool.c \
	adaptor-uisp.c usb-helper.c  

rf24tool: $(SRCS)
	$(CROSS_COMPILE)gcc $(CFLAGS)  -Wall -I"../include" -g -o $(@) $(^) $(LDFLAGS) -lusb


install: rf24tool
	install -D rf24tool $(DESTDIR)/usr/bin/

maketestdata: 
	@echo "Generating test data"
	@dd if=/dev/urandom of=testdata/test-1k.bin   bs=1 count=1024 
	@dd if=/dev/urandom of=testdata/test-900b.bin bs=1 count=900 
	@dd if=/dev/urandom of=testdata/test-4k.bin   bs=1 count=4096
	@dd if=/dev/urandom of=testdata/test-full.bin   bs=1 count=28672	 

define test
	./rf24tool --part $(1) --file ./testdata/$(2) --write
endef

test-m328p: maketestdata
	@echo "Running bootloader tests"
	$(call test,eeprom,test-1k.bin)
	$(call test,eeprom,test-900b.bin)
	$(call test,flash,test-1k.bin)
	$(call test,flash,test-900b.bin)
	$(call test,flash,test-4k.bin)
	$(call test,flash,test-full.bin)

clean:
	-rm rf24tool
