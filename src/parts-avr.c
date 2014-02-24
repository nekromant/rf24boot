#include <arch/antares.h>
#include <arch/delay.h>
#include <rf24boot.h>

#define DEBUG_LEVEL 5
#define COMPONENT "avrparts"

#include <lib/printk.h>
#include <lib/RF24.h>
#include <lib/panic.h>
#include <avr/eeprom.h>
#include <avr/boot.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <string.h>


#ifdef CONFIG_HAS_EEPROM_PART
#define EEPROM_IOSIZE 16

int do_eeprom_read(struct rf24boot_partition* part, struct rf24boot_data *dat) 
{
	uint8_t *eptr = (uint8_t *) (uint16_t) dat->addr;
	if (eptr >= ((uint8_t*) (uint16_t) part->info.size))
		return 0; 
	eeprom_read_block(dat->data, eptr, part->info.iosize);
	return part->info.iosize; 	
}

void do_eeprom_write(struct rf24boot_partition* part, struct rf24boot_data *dat) 
{
	uint8_t *eptr = (uint8_t *) (uint16_t) dat->addr;
	eeprom_busy_wait();
	eeprom_write_block(dat->data, eptr, part->info.iosize);
}


struct rf24boot_partition eeprom_part = {
	.info = { 
		.name = "eeprom",
		.size = E2END + 1,
		.iosize   = EEPROM_IOSIZE,
	},
	.read = do_eeprom_read,
	.write = do_eeprom_write
};
BOOT_PARTITION(eeprom_part);

#endif

#ifdef CONFIG_HAS_FLASH_PART

int do_flash_read(struct rf24boot_partition* part, struct rf24boot_data *dat) 
{
	if (dat->addr >= part->info.size) 
		return 0;
	memcpy_PF(dat->data, dat->addr, part->info.iosize);
	return part->info.iosize;
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

static unsigned char pbuf[SPM_PAGESIZE]; 
void do_flash_write(struct rf24boot_partition* part, struct rf24boot_data *dat) 
{
	uint16_t *data = (uint16_t *) dat->data;
	uint32_t offset = dat->addr & (SPM_PAGESIZE-1);
	memcpy(&pbuf[offset], (uint8_t*) data, part->info.iosize);
	if (0 == ((dat->addr + part->info.iosize) & (SPM_PAGESIZE - 1))) {	
		boot_program_page (dat->addr & (~(SPM_PAGESIZE-1)), pbuf);
	}
}

#ifdef CONFIG_AVR_BLDADDR
#define BOOT_RESERVED (FLASHEND - CONFIG_AVR_BLDADDR + 1)
#else 
/* Not in bootloader? Assume some debugging scenario and expose everything */
#define BOOT_RESERVED  0
#endif

struct rf24boot_partition flash_part = {
	.info = { 
		.name = "flash",
		.pad = SPM_PAGESIZE, 
		.size = ((uint32_t) FLASHEND - BOOT_RESERVED) + 1,
		.iosize   = 16, /* SPM_PAGE_SIZE % iosize == 0 */
	},
	.read = do_flash_read,
	.write = do_flash_write
};
BOOT_PARTITION(flash_part);


#endif
