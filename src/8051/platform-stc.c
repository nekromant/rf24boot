#include <arch/antares.h> 
#include <arch/delay.h>
#include <lib/RF24.h>
#include <string.h>

#define COMPONENT "rf24boot"
#define DEBUG_LEVEL CONFIG_DEBUGGING_VERBOSITY

#include <lib/printk.h>
#include <lib/panic.h>
#include <rf24boot.h>
#include <arch/stc.h>
#include <arch/stc/iap.h>



void rf24boot_boot_partition(struct rf24boot_partition *part)
{
	
}

static void set_csn(int level) 
{
}

static void set_ce(int level) 
{
}


static void spi_set_speed(int speed)
{
	dbg("spi: speed change to %d\n", speed);
}

static uint8_t spi_xfer(uint8_t data)
{
}

static struct rf24 r = {
	.csn = set_csn,
	.ce  = set_ce, 
	.spi_set_speed = spi_set_speed, 
	.spi_xfer = spi_xfer
};

struct rf24 *g_radio = &r;

