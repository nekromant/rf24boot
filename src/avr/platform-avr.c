/*
 *  Universal rf24boot bootloader : avr hardspi
 *  Copyright (C) 2014  Andrew 'Necromant' Andrianov <andrew@ncrmnt.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <arch/antares.h> 
#include <avr/io.h> 
#include <arch/delay.h>
#include <lib/RF24.h>
#include <string.h>
#include <avr/boot.h>
#include <avr/io.h>
#include <avr/wdt.h>

#define COMPONENT "rf24boot"
#define DEBUG_LEVEL CONFIG_DEBUGGING_VERBOSITY

#include <lib/printk.h>
#include <lib/panic.h>
#include <rf24boot.h>


#define CSN_PIN   (1<<CONFIG_CSN_PIN)
#define  CE_PIN   (1<<CONFIG_CE_PIN)
#define SPI_PORTX PORTB
#define SPI_DDRX  DDRB
#define SPI_MOSI  CONFIG_SPI_MOSI_PIN
#define SPI_MISO  CONFIG_SPI_MISO_PIN
#define SPI_SCK   CONFIG_SPI_SCK_PIN
#define SPI_SS    CONFIG_SPI_SS_PIN

static void set_csn(int level) 
{
	dbg("csn: %d\n", level)
	if (level) 
		CSN_PORT |= CSN_PIN;
	else
		CSN_PORT &= ~CSN_PIN;
}

static void set_ce(int level) 
{
	dbg("ce: %d\n", level);
	if (level) 
		CE_PORT |= CE_PIN;
	else
		CE_PORT &= ~CE_PIN;
}


static void spi_set_speed(int speed)
{
	dbg("spi: speed change to %d\n", speed);
}

static void spi_write(const uint8_t *data, uint8_t len)
{
	while(len--) {
		SPDR = *data++;
		while(!(SPSR & (1<<SPIF)));
	}
}

static void spi_read(uint8_t *data, uint8_t len)
{
	while(len--) {
		SPDR = 0xff;
		while(!(SPSR & (1<<SPIF)));
		*data++ = SPDR;
	}
}


static struct rf24 r = {
	.csn = set_csn,
	.ce  = set_ce, 
	.spi_set_speed = spi_set_speed, 
	.spi_write = spi_write,
	.spi_read  = spi_read
};

struct rf24 *g_radio = &r;

void rf24boot_platform_reset()
{
	wdt_enable(WDTO_30MS);
	while(1) ;;;
}


ANTARES_INIT_LOW(platform_setup)
{
	wdt_disable();
	dbg("setting up rf io pins\n");
	CSN_DDR |= CSN_PIN;
	CE_DDR  |=  CE_PIN;

	dbg("spi: init\n");
	SPI_DDRX &= ~(1<<SPI_MISO);
	SPI_DDRX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS);
	SPI_PORTX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_MISO)|(1<<SPI_SS);
	

	SPCR = ((1<<SPE)|               /* SPI Enable */
		(0<<SPIE)|              /* SPI Interrupt Enable */
		(0<<DORD)|              /* Data Order (0:MSB first / 1:LSB first) */
		(1<<MSTR)|              /* Master/Slave select */   
		(0<<SPR1)|(0<<SPR0)|    /* SPI Clock Rate */
		(0<<CPOL)|              /* Clock Polarity (0:SCK low / 1:SCK hi when idle) */
		(0<<CPHA));             /* Clock Phase (0:leading / 1:trailing edge sampling) */

	SPSR = (1<<SPI2X);              /* Double Clock Rate */

}


