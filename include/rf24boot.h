#ifndef RF24BOOT_H
#define RF24BOOT_H


enum rf24boot_op {
	rf24boot_op_read = 0, 
	rf24boot_op_write = 1
};

enum rf24boot_mem {
	rf24boot_mem_flash = 0, 
	rf24boot_mem_eeprom = 1
};


#define RF24_MEM_TYPE  1
#define RF24_MEM_OP    0 

#define RF24_PING      0 




struct rf24boot_cmd
{
	uint8_t  flags;
	uint32_t addr;
	uint32_t data;
};


#endif
