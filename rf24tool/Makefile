CFLAGS  = $(shell pkg-config --cflags libusb-1.0)
LDFLAGS = $(shell pkg-config --libs libusb-1.0)

all: rf24tool

OBJS=usb.c main.c rf24.c
rf24tool: $(OBJS)
	gcc $(CFLAGS)  -Wall -I"../include" -o $(@) $(^) $(LDFLAGS) -lusb

clean:
	-rm rf24tool
