#ifndef RF24BOOT_H
#define RF24BOOT_H


enum rf24boot_op {
	RF_OP_HELLO  = 0,
	RF_OP_BOOT   = 1,
	RF_OP_READ   = 2, 
	RF_OP_WRITE  = 3
};

enum rf24boot_mem {
	rf24boot_mem_flash  = 0, 
	rf24boot_mem_eeprom = 1
};


struct rf24boot_cmd
{
	uint8_t  op;
	uint8_t  mem;
	uint32_t addr;
	uint32_t data;	
};

struct rf24boot_hello_resp {
	uint32_t flashsz;
	uint32_t eepromsz;
	char id[];
};

extern struct rf24 *g_radio;

#endif
