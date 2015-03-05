/*
 *  Universal rf24boot bootloader : Common avr code
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

#define DEBUG_LEVEL CONFIG_DEBUGGING_VERBOSITY
#define COMPONENT "avrparts"

#include <lib/printk.h>
#include <lib/RF24.h>
#include <lib/panic.h>
#include <avr/eeprom.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <avr/wdt.h>

#include <rf24boot.h>

#define EEPROM_IOSIZE 16
#define FLASH_IOSIZE 16

#ifdef CONFIG_AVR_BLDADDR
 #define BOOT_RESERVED (FLASHEND - CONFIG_AVR_BLDADDR + 1)
 #define FLASH_SIZE    (FLASHEND - BOOT_RESERVED)
#else 
/* Not in bootloader? Disable flash! */
 #if defined(CONFIG_HAS_FLASH_PART)
 #undef CONFIG_HAS_FLASH_PART
 #warning "Will not enable flash partition when we're not in bootloader"
 #endif
#endif

static void (*nullVector)(void) __attribute__((__noreturn__));
static unsigned char pbuf[SPM_PAGESIZE]; 

#ifdef MCUCR
#define CR MCUCR
#endif

#ifdef GICR
#define CR GICR
#endif

/* AVR can have only one bootable partition, so we ignore the argument */
void rf24boot_boot_partition(struct rf24boot_partition *part)
{
        cli();
        boot_rww_enable();

#if 0
        CR = (1 << IVCE); /* enable change of interrupt vectors */
        CR = (0 << IVSEL); /* move interrupts to application flash section */
#endif

/* We must go through a global function pointer variable instead of writing
 * ((void (*)(void))0)();
 * because the compiler optimizes a constant 0 to "rcall 0" which is not
 * handled correctly by the assembler.
 */
#ifdef CONFIG_WDT_DISABLE
	wdt_disable();
#endif
        nullVector();
}

/* Finally, let's register our partitions */

#ifdef CONFIG_HAS_EEPROM_PART


int do_eeprom_read(struct rf24boot_partition* part, uint32_t addr, unsigned char* buf) 
{
	uint8_t *eptr = (uint8_t *) (uint16_t) addr;
	if (eptr >= ((uint8_t*) (uint16_t) (E2END + 1)))
		return 0; 
	eeprom_read_block(buf, eptr, EEPROM_IOSIZE);
	return EEPROM_IOSIZE; 	
}

void do_eeprom_write(struct rf24boot_partition* part, uint32_t addr, const unsigned char* buf) 
{
	uint8_t *eptr = (uint8_t *) (uint16_t) addr;
	eeprom_write_block(buf, eptr, EEPROM_IOSIZE);
}


struct rf24boot_partition eeprom_part = {
	.info = { 
		.name = "eeprom",
		.pad  = 1, 
		.size = E2END + 1,
		.iosize   = EEPROM_IOSIZE,
	},
	.read = do_eeprom_read,
	.write = do_eeprom_write
};
BOOT_PARTITION(eeprom_part);

#endif

#ifdef CONFIG_HAS_FLASH_PART

int do_flash_read(struct rf24boot_partition* part, uint32_t addr, unsigned char* buf) 
{
	if (addr >= (FLASH_SIZE + 1)) 
		return 0;
	memcpy_PF(buf, addr, FLASH_IOSIZE);
	return FLASH_IOSIZE;
}

/* TODO: 
   We might spare a few bytes and don't buffer the whole page in RAM.
   But I was too lazy.
 */
inline void boot_program_page (uint32_t page, uint8_t *buf)
{
        uint16_t i;
        uint8_t sreg;

        // Disable interrupts.

        sreg = SREG;
        cli();
    
        eeprom_busy_wait ();

        boot_page_erase (page);
        boot_spm_busy_wait ();      // Wait until the memory is erased.

        for (i=0; i<SPM_PAGESIZE; i+=2)
        {
		// Set up little-endian word.

		uint16_t w = *buf++;
		w += (*buf++) << 8;
        
		boot_page_fill (page + i, w);
        }

        boot_page_write (page);     // Store buffer in flash page.
        boot_spm_busy_wait();       // Wait until the memory is written.

        // Reenable RWW-section again. We need this if we want to jump back
        // to the application after bootloading.

        boot_rww_enable ();

        // Re-enable interrupts (if they were ever enabled).

        SREG = sreg;
}


void do_flash_write(struct rf24boot_partition* part, uint32_t addr, const unsigned char* buf) 
{
	uint32_t offset = addr & (SPM_PAGESIZE-1);
	memcpy(&pbuf[offset], buf, FLASH_IOSIZE);
	if (0 == ((addr + FLASH_IOSIZE) & (SPM_PAGESIZE - 1))) {	
		boot_program_page (addr & (~(SPM_PAGESIZE-1)), pbuf);
	}
}


struct rf24boot_partition flash_part = {
	.info = { 
		.name = "flash",
		.pad = SPM_PAGESIZE, 
		.size = ((uint32_t) FLASH_SIZE) + 1,
		.iosize   = 16, /* SPM_PAGE_SIZE % iosize == 0 */
	},
	.read = do_flash_read,
	.write = do_flash_write
};
BOOT_PARTITION(flash_part);


#endif


#ifdef CONFIG_WDT_ENABLE
ANTARES_INIT_LOW(avr_wdt)
{
/* We don't use interrupts here, so it's not needed */
#if 0
        CR = (1 << IVCE); /* enable change of interrupt vectors */
        CR = (1 << IVSEL); /* move interrupts to boot flash section */
#endif
	wdt_enable(WDTO_2S);
}
#endif


#define TIMEOUT (CONFIG_BOOTCOND_TIMEOUT / 50)

static int boot_timer = 0;


#if defined(CONFIG_BOOTCOND_TIMED_AVR) || defined(CONFIG_WDT_ENABLE)
ANTARES_APP(bootcond)
{
#ifdef CONFIG_WDT_ENABLE
	wdt_reset();
#endif

#ifdef CONFIG_BOOTCOND_TIMED_AVR
	if (!g_rf24boot_got_hello)
	{
		delay_ms(50);
		if (boot_timer++ > TIMEOUT) 
		{
			dbg("It's boot time!\n");
			rf24boot_boot_partition(NULL); /* AVR Doesn't care, save space */
		}
	}	
#endif
}
#endif


void rf24boot_platform_reset()
{
	wdt_enable(WDTO_30MS);
	while(1) ;;;
}
