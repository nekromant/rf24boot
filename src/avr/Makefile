

objects-$(CONFIG_ROLE_SLAVE)+=avr-boot-common.o

objects-$(CONFIG_BOOTCOND_TIMED_AVR)+=bootcond-timed-avr.o

cflags-y+=-DBOOTLOCK_PORTX=PORT$(call unquote,$(CONFIG_BOOTLOCK_PORT))
cflags-y+=-DBOOTLOCK_PINX=PIN$(call unquote,$(CONFIG_BOOTLOCK_PORT))
objects-$(CONFIG_BOOTLOCK_AVR)+=bootlock-avr.o
