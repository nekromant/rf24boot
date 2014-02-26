#include <arch/antares.h>
#include <arch/delay.h>
#include <rf24boot.h>

#define DEBUG_LEVEL 5
#define COMPONENT "lightctlhacks"

#include <lib/printk.h>
#include <lib/RF24.h>
#include <lib/panic.h>
#include <avr/eeprom.h>
#include <string.h>

ANTARES_INIT_LOW(switch_off_lights)
{
	DDRB|=(1<<1|1<<2|1<<3); 
	PORTB&=~(1<<1|1<<2|1<<3);
	DDRD|=(1<<3|1<<5|1<<6);
	DDRD&=~(1<<3|1<<5|1<<6);
}
