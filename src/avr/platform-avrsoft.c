/*
 *  Universal rf24boot bootloader : avr softspi part
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
#include <avr/wdt.h>
#include <avr/sfr_defs.h>

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
	if (level) 
		CSN_PORT |= CSN_PIN;
	else
		CSN_PORT &= ~CSN_PIN;
}

static void set_ce(int level) 
{
	if (level) 
		CE_PORT |= CE_PIN;
	else
		CE_PORT &= ~CE_PIN;
}


static void spi_set_speed(int speed)
{
	dbg("spi: speed change to %d\n", speed);
}

#define SCK_ZERO  SPI_PORTX &= ~(1<<SPI_SCK)
#define SCK_ONE   SPI_PORTX |=  (1<<SPI_SCK)
#define MOSI_ZERO SPI_PORTX &= ~(1<<SPI_MOSI)
#define MOSI_ONE  SPI_PORTX |=  (1<<SPI_MOSI)




#define sbi(sfr, bit) sfr |= _BV(bit)
#define cbi(sfr, bit) sfr &= ~_BV(bit)

static uint8_t spi_xfer(uint8_t data)
{
	uint8_t recv=0;
	int i;
	for (i=7; i>=0; i--) {
		SCK_ZERO;
		if (data & (1<<i))
			MOSI_ONE;
		else
			MOSI_ZERO;
		SCK_ONE;
		recv |= (SPI_PINX & (1<<SPI_MISO)) ? (1 << i) : 0;
	}
	SCK_ZERO;
	return recv;
}
static struct rf24 r = {
	.csn = set_csn,
	.ce  = set_ce, 
	.spi_set_speed = spi_set_speed, 
	.spi_xfer = spi_xfer
};

struct rf24 *g_radio = &r;

void rf24boot_platform_reset()
{
	wdt_enable(WDTO_4S);
	while(1) ;;
}


ANTARES_INIT_LOW(platform_setup)
{
	
	wdt_reset();
	MCUSR &= ~(1<<WDRF);
	wdt_disable(); 
	
	dbg("setting up rf io pins\n");

	CSN_DDR |= CSN_PIN;
	CE_DDR  |=  CE_PIN;

	SPI_DDRX |= (1<<SPI_MOSI) | (1<<SPI_SCK) |(1<<SPI_SS);
	SPI_DDRX &= ~(1<<SPI_MISO);
	SPI_PORTX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS);
	SCK_ZERO;
}
