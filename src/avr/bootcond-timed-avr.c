/*
 *  Universal rf24boot bootloader : Optimized timed bootcond for avr
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
			rf24boot_boot_partition(NULL); /* AVR Doesn't care, save space */
		}
	}	
}

