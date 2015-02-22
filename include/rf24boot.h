#ifndef RF24BOOT_H
#define RF24BOOT_H

#ifdef CONFIG_TOOLCHAIN_SDCC
#define PACKED 
#else
#define PACKED __attribute__ ((packed))
#endif

enum rf24boot_op {
	RF_OP_HELLO     = 0,
	RF_OP_PARTINFO,
	RF_OP_BOOT,
	RF_OP_READ,
	RF_OP_WRITE,
	RF_OP_NOP,
};

#define RF24BOOT_MAX_IOSIZE 26

#define RF24BOOT_CMDSIZE(iosize)					\
	(sizeof(struct rf24boot_cmd) - RF24BOOT_MAX_IOSIZE + iosize)

/* This generic packet is just a cont. counter + opcode */
struct rf24boot_cmd
{
	uint8_t  op; /* opcode */
	uint8_t data[31];
} PACKED;

/* for read & write */
struct rf24boot_data
{
	uint8_t  part;
	uint32_t addr;
	uint8_t data[RF24BOOT_MAX_IOSIZE];
} PACKED;

/* response to a hello packet */
struct rf24boot_hello_resp {
	uint8_t numparts; 
	uint8_t is_big_endian;
	char id[29];
} PACKED;

/* op_partinfo */
struct rf24boot_partition_header {
	uint8_t iosize;
	uint32_t size; 
	uint16_t pad; 
	char name[8];
} PACKED;

struct rf24boot_partition {
	struct rf24boot_partition_header info;
	int (*read)(struct rf24boot_partition* part, struct rf24boot_data *dat);
	void (*write)(struct rf24boot_partition* part, struct rf24boot_data *dat); 
};
extern struct rf24 *g_radio;

void rf24boot_add_part(struct rf24boot_partition *part);
void rf24boot_boot_partition(struct rf24boot_partition *part);
void rf24boot_boot_by_name(char* name);
uint8_t rf24boot_got_hello();
void rf24boot_platform_reset();


#define BOOT_PARTITION(part) ANTARES_INIT_LOW(addpart ## part) \
	{ \
	rf24boot_add_part( &part ) ; \
	}

#endif
