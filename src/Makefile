objects-$(CONFIG_ROLE_SLAVE)+=slave.o
objects-$(CONFIG_ROLE_UISP)+=uisp-app.o

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

ifeq ($(CONFIG_ARCH_AVR),y)
objects-$(CONFIG_ROLE_SLAVE)+=parts-avr.o boot-avr.o 

objects-$(CONFIG_AVR_HARDSPI)+=platform-avr.o
objects-$(CONFIG_AVR_SOFTSPI)+=platform-avrsoft.o

endif
