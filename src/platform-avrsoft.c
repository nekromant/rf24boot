#include <arch/antares.h> 
#include <avr/io.h> 
#include <arch/delay.h>
#include <lib/RF24.h>
#include <string.h>
#include <avr/boot.h>
#include <avr/wdt.h>
#include <avr/sfr_defs.h>

#define COMPONENT "rf24boot"
#define DEBUG_LEVEL 0

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
	wdt_enable(WDTO_30MS);
	while(1) ;;;
}


ANTARES_INIT_LOW(platform_setup)
{
	wdt_disable(); /* Make sure wdt is disabled in case app enables it */
	dbg("setting up rf io pins\n");

	CSN_DDR |= CSN_PIN;
	CE_DDR  |=  CE_PIN;

	SPI_DDRX |= (1<<SPI_MOSI) | (1<<SPI_SCK) |(1<<SPI_SS);
	SPI_DDRX &= ~(1<<SPI_MISO);
	SPI_PORTX |= (1<<SPI_MOSI)|(1<<SPI_SCK)|(1<<SPI_SS);
	SCK_ZERO;


}


