CPPFLAGS  += $(shell pkg-config --cflags libusb-1.0)
CPPFLAGS  +=-MD -MP -std=c++0x -Iinclude -Wall -g
LDFLAGS = $(shell pkg-config --libs libusb-1.0)

objects+=\
	rf24libusbadaptor.o \
	rf24packet.o \
	rf24transfer.o \
	rf24popentransfer.o \
	rf24iotransfer.o \
	rf24conftransfer.o \
	rf24adaptor.o \
	main.o

all: test

-include $(objects:%.o=%.d)

test: $(objects)
	g++ $(LDFLAGS) -o $(@) $^


%.o: %.cpp
	g++ -c $(CPPFLAGS) -o $(@) $<

clean: 
	rm -f *.o *~ test *.d
