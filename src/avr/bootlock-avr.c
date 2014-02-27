/*
 *  Universal rf24boot bootloader : bootlock for avr
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
#include <rf24boot.h>

#define BOOTLOCK_PIN   (1<<CONFIG_BOOTLOCK_PIN)

ANTARES_INIT_LOW(bootlock) {
	BOOTLOCK_PORTX|=BOOTLOCK_PIN;
	if (!(BOOTLOCK_PINX & BOOTLOCK_PIN)) 
		rf24boot_boot_partition(NULL);
}


