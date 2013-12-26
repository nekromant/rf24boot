#ifndef RF24BOOT_H
#define RF24BOOT_H

enum rf24boot_op {
	RF_OP_HELLO     = 0,
	RF_OP_PARTINFO,
	RF_OP_BOOT,
	RF_OP_READ,
	RF_OP_WRITE,
	RF_OP_NOP, /* For reading out acks */
};

#define RF24BOOT_MAX_IOSIZE 26

#define RF24BOOT_CMDSIZE(iosize)					\
	(sizeof(struct rf24boot_cmd) - RF24BOOT_MAX_IOSIZE + iosize)

struct rf24boot_cmd
{
	uint8_t  op;
	uint8_t  part;
	uint32_t addr;
	char data[RF24BOOT_MAX_IOSIZE];
} __attribute__ ((packed));

struct rf24boot_hello_resp {
	uint8_t numparts; 
	uint8_t is_big_endian;
	char id[30];
} __attribute__ ((packed));

struct rf24boot_partition_header {
	uint8_t iosize;
	uint32_t size; 
	char name[16];
} __attribute__ ((packed));

struct rf24boot_partition {
	struct rf24boot_partition_header info;
	void (*read)(struct rf24boot_partition* part, struct rf24boot_cmd *cmd);
	void (*write)(struct rf24boot_partition* part, struct rf24boot_cmd *cmd); 
};
extern struct rf24 *g_radio;

void rf24boot_add_part(struct rf24boot_partition *part);
void rf24boot_boot_partition(struct rf24boot_partition *part);

#define BOOT_PARTITION(part) ANTARES_INIT_LOW(addpart ## part) \
	{ \
	rf24boot_add_part(&part); \
	}

#endif
