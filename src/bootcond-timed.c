#include <arch/antares.h>
#include <arch/delay.h>
#include <rf24boot.h>

#define DEBUG_LEVEL 5
#define COMPONENT "rf24boot"

#include <lib/printk.h>
#include <lib/RF24.h>
#include <lib/panic.h>
#include <avr/eeprom.h>
#include <string.h>

#define TIMEOUT (CONFIG_BOOTCOND_TIMEOUT / 50)

static int boot_timer = 0;


ANTARES_APP(bootcond_timed)
{
	if (!rf24boot_got_hello())
	{
		delay_ms(50);
		if (boot_timer++ > TIMEOUT) 
		{
			rf24boot_boot_by_name(CONFIG_DEFAULT_BOOTPART);
		}
	}	
}

