CPPFLAGS  += $(shell pkg-config --cflags libusb-1.0)
CPPFLAGS  +=-MD -MP -std=c++0x -Iinclude -Wall -g -DELPP_DISABLE_DEBUG_LOGS
LDFLAGS = $(shell pkg-config --libs libusb-1.0)

objects+=\
	rf24libusbadaptor.o \
	rf24packet.o \
	rf24transfer.o \
	rf24popentransfer.o \
	rf24sweeptransfer.o \
	rf24iotransfer.o \
	rf24conftransfer.o \
	rf24adaptor.o \



all: test rf24-sweep rf24-load

-include $(objects:%.o=%.d)

test: $(objects) main.o
	g++ $(LDFLAGS) -o $(@) $^

rf24-sweep: $(objects) rf24-sweep.o
	g++ $(LDFLAGS) -o $(@) $^

rf24-load: $(objects) rf24-load.o
	g++ $(LDFLAGS) -o $(@) $^

%.o: %.cpp
	g++ -c $(CPPFLAGS) -o $(@) $<

clean: 
	rm -f *.o *~ test *.d
