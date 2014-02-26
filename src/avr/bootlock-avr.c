#include <arch/antares.h> 
#include <avr/io.h> 
#include <arch/delay.h>
#include <lib/RF24.h>
#include <string.h>
#include <avr/boot.h>
#include <avr/wdt.h>
#include <avr/sfr_defs.h>
#include <rf24boot.h>

#define BOOTLOCK_PIN   (1<<CONFIG_BOOTLOCK_PIN)

ANTARES_INIT_LOW(bootlock) {
	BOOTLOCK_PORTX|=BOOTLOCK_PIN;
	if (!(BOOTLOCK_PINX & BOOTLOCK_PIN)) 
		rf24boot_boot_partition(NULL);
}


