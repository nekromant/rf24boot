-include blackjack.mk
CPPFLAGS  += $(shell pkg-config --cflags libusb-1.0)
CPPFLAGS  +=-MD -MP -std=c++0x -Iinclude -Wall -g 
CPPFLAGS  += -DELPP_DISABLE_DEBUG_LOGS
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
	$(SILENT_LD)$(CXX) $(LDFLAGS) -o $(@) $^

rf24-sweep: $(objects) rf24-sweep.o
	$(SILENT_LD)$(CXX) $(LDFLAGS) -o $(@) $^

rf24-load: $(objects) rf24ptable.o rf24-load.o
	$(SILENT_LD)$(CXX) $(LDFLAGS) -o $(@) $^

%.o: %.cpp
	$(SILENT_CXX)$(CXX) -c $(CPPFLAGS) -o $(@) $<

clean: 
	rm -f *.o *~ test *.d
