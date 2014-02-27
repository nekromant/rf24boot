/*
 *  Universal rf24boot bootloader
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

#define COMPONENT "rf24master"

#include <lib/printk.h>
#include <lib/RF24.h>


static char hw_addr[5] = { 0xff, 0xff, 0xff, 0xff, 0xfa };

static struct rf24boot_cmd cmd;


ANTARES_APP(master)
{
	int ret; 
	dbg("Wireless in master mode\n");
	rf24_open_writing_pipe(g_radio, hw_addr);

	printk("Searching target...");
	while (1) { 
		cmd.op = RF_OP_HELLO;
		ret = rf24_write(g_radio, &cmd, sizeof(cmd));
		printk(".");
		if (ret == 0) 
			break; 
		delay_ms(1000);
	}
	while(1) ;;
}
