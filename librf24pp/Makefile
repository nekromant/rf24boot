-include blackjack.mk
-include $(objects:%.o=%.d)

utils+=test
objects-test+=main.o
utils+=rf24-sweep
objects-rf24-sweep+=rf24-sweep.o
utils+=rf24-boot
objects-rf24-boot+=rf24-load.o rf24ptable.o


#Developer mode. TODO: Production mode with pkg-config
define util_rule
$(1): $$(objects-$(1)) $(objects)
	$$(SILENT_LD)$$(CXX) $$(LDFLAGS) -o $$(@) $$^

uobjects+=$$(objects-$(1))
endef

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

%.o: %.cpp
	$(SILENT_CXX)$(CXX) -c $(CPPFLAGS) -o $(@) $<


all: $(utils)

$(foreach u,$(utils),$(eval $(call util_rule,$(u))))

clean: 
	$(SILENT_CLEAN) rm -f $(utils) $(addsuffix .d,$(objects)) $(objects) $(uobjects)