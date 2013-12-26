#include <arch/antares.h>
#include <arch/delay.h>
#include <rf24boot.h>

#define DEBUG_LEVEL 5
#define COMPONENT "avrparts"

#include <lib/printk.h>
#include <lib/RF24.h>
#include <lib/panic.h>
#include <avr/eeprom.h>
#include <string.h>


#ifdef CONFIG_HAS_EEPROM_PART
void do_eeprom_read(struct rf24boot_partition* part, struct rf24boot_cmd *cmd) {
	//cmd->data8 = eeprom_read_byte((uint8_t*) (uint16_t) cmd->addr);
}

void do_eeprom_write(struct rf24boot_partition* part, struct rf24boot_cmd *cmd) {
	//eeprom_write_byte((uint8_t*) (uint16_t) cmd->addr, cmd->data8);
}


struct rf24boot_partition eeprom_part = {
	.info = { 
		.name = "eeprom",
		.size = E2END + 1,
		.iosize   = 1,
	},
	.read = do_eeprom_read,
	.write = do_eeprom_write
};
BOOT_PARTITION(eeprom_part);

#endif

#ifdef CONFIG_HAS_FLASH_PART

struct rf24boot_partition flash_part = {
	.info = { 
		.name = "flash",
		.size = ((uint32_t) FLASHEND) + 1,
		.iosize   = 2,
	},
	.read = do_eeprom_read,
	.write = do_eeprom_write
};
BOOT_PARTITION(flash_part);


#endif
