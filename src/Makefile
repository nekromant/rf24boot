objects-$(CONFIG_ROLE_SLAVE)+=slave.o
objects-$(CONFIG_ROLE_SENDER)+=packet-sender.o
objects-$(CONFIG_ROLE_LISTENER)+=packet-listener.o
objects-$(CONFIG_ROLE_UISP)+=uisp-app.o cb.o


subdirs-$(CONFIG_ARCH_AVR)+=avr/
subdirs-$(CONFIG_ARCH_8051)+=8051/

subdirs-y+=hacks/

objects-$(CONFIG_BOOTCOND_TIMED)+=bootcond-timed.o
objects-$(CONFIG_PART_DUMMY)+=dummypart.o

subdirs-$(CONFIG_ROLE_USERSPACE)+= native/

