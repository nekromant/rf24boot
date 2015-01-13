CFLAGS  = $(shell pkg-config --cflags libusb-1.0)
LDFLAGS = $(shell pkg-config --libs libusb-1.0)

MAKEFLAGS+=--no-print-directory -r

CROSS_COMPILE?=

UTILS+=rf24-sweep rf24-listen rf24-send rf24-loader
#UTILS+=rf24-listen

all: $(UTILS)

SRCS=   adaptor-factory.c librf24.c \
	adaptor-asyncusb.c helpers.c

define UTIL_RULE
$(1) : $(SRCS) $(1).c
	$(CROSS_COMPILE)$(CC) $(CFLAGS)  -Wall -I"../include" -g -o $$(@) $$(^) $(LDFLAGS) -ldl -lusb
endef


$(foreach util,$(UTILS),$(eval $(call UTIL_RULE,$(util))))



install: rf24tool
	install -D rf24tool $(DESTDIR)/usr/bin/

testdata: 
	@echo "Generating test data"
	@mkdir -p testdata
	@dd if=/dev/urandom of=testdata/test-1k.bin   bs=1 count=1024 
	@dd if=/dev/urandom of=testdata/test-900b.bin bs=1 count=900 
	@dd if=/dev/urandom of=testdata/test-4k.bin   bs=1 count=4096
	@dd if=/dev/urandom of=testdata/test-full.bin   bs=1 count=28672	 

define test
	./rf24tool --part $(1) --file ./testdata/$(2) --write
endef

test-m328p-eeprom: testdata
	@echo "Running bootloader tests"
	$(call test,eeprom,test-1k.bin)
	$(call test,eeprom,test-900b.bin)
test-m328p-flash: testdata
	$(call test,flash,test-1k.bin)
	$(call test,flash,test-900b.bin)
	$(call test,flash,test-4k.bin)
	$(call test,flash,test-full.bin)

clean:
	-rm $(UTILS)
