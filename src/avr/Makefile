
cflags-y+=-DCSN_PORT=PORT$(call unquote,$(CONFIG_CSN_PORT))
cflags-y+=-DCE_PORT=PORT$(call unquote,$(CONFIG_CE_PORT))
cflags-y+=-DCSN_DDR=DDR$(call unquote,$(CONFIG_CSN_PORT))
cflags-y+=-DCE_DDR=DDR$(call unquote,$(CONFIG_CE_PORT))

cflags-y+=-DSPI_PORTX=PORT$(call unquote,$(CONFIG_SPI_PORT))
cflags-y+=-DSPI_DDRX=DDR$(call unquote,$(CONFIG_SPI_PORT))
cflags-y+=-DSPI_PINX=PIN$(call unquote,$(CONFIG_SPI_PORT))


cflags-y+=-DCE_PORT=PORT$(call unquote,$(CONFIG_CE_PORT))
cflags-y+=-DCSN_DDR=DDR$(call unquote,$(CONFIG_CSN_PORT))
cflags-y+=-DCE_DDR=DDR$(call unquote,$(CONFIG_CE_PORT))

objects-$(CONFIG_ROLE_SLAVE)+=avr-boot-common.o

objects-$(CONFIG_BOOTCOND_TIMED_AVR)+=bootcond-timed-avr.o

objects-$(CONFIG_AVR_HARDSPI)+=platform-avr.o
objects-$(CONFIG_AVR_SOFTSPI)+=platform-avrsoft.o

cflags-y+=-DBOOTLOCK_PORTX=PORT$(call unquote,$(CONFIG_BOOTLOCK_PORT))
cflags-y+=-DBOOTLOCK_PINX=PIN$(call unquote,$(CONFIG_BOOTLOCK_PORT))
objects-$(CONFIG_BOOTLOCK_AVR)+=bootlock-avr.o
