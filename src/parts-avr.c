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

int do_eeprom_read(struct rf24boot_partition* part, struct rf24boot_data *dat) 
{
	uint8_t *eptr = (uint8_t *) (uint16_t) dat->addr;
	if (eptr >= (uint8_t*) part->info.size)
		return 0; 
	eeprom_read_block(dat->data, eptr, part->info.iosize);
	return part->info.iosize; 	
}

void do_eeprom_write(struct rf24boot_partition* part, struct rf24boot_data *dat) 
{
	uint8_t *eptr = (uint8_t *) (uint16_t) dat->addr;
	eeprom_busy_wait();
	eeprom_write_block(dat->data, eptr, RF24BOOT_MAX_IOSIZE);
}


struct rf24boot_partition eeprom_part = {
	.info = { 
		.name = "eeprom",
		.size = E2END + 1,
		.iosize   = 16,
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

void do_flash_write(struct rf24boot_partition* part, struct rf24boot_data *dat) 
{
	uint16_t *data = (uint16_t *) dat->data;
	uint8_t i;
	uint8_t sreg;
	sreg = SREG;
        cli();
        eeprom_busy_wait ();
        boot_spm_busy_wait();       /* Wait for prev. write */
	if (0 == (dat->addr % SPM_PAGESIZE)) {
	        boot_page_erase (dat->addr);
		boot_spm_busy_wait (); 				
	}
	
	for (i=0; i < part->info.iosize; i+=2) {
		boot_page_fill(dat->addr, *data++);
		dat->addr += 2;
	}

	if (0 == (dat->addr % SPM_PAGESIZE))	
		boot_page_write (dat->addr - SPM_PAGESIZE);

	boot_rww_enable ();
	SREG = sreg;
}

struct rf24boot_partition flash_part = {
	.info = { 
		.name = "flash",
		.pad = SPM_PAGESIZE, 
		.size = ((uint32_t) FLASHEND) + 1,
		.iosize   = 16, /* SPM_PAGE_SIZE % iosize == 0 */
	},
	.read = do_flash_read,
	.write = do_flash_write
};
BOOT_PARTITION(flash_part);


#endif
