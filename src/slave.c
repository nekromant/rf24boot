#include <arch/antares.h>
#include <arch/delay.h>
#include <rf24boot.h>

#define DEBUG_LEVEL 0
#define COMPONENT "rf24boot"

#include <lib/printk.h>
#include <lib/RF24.h>
#include <lib/panic.h>
#include <avr/eeprom.h>
#include <string.h>


#define CONFIG_BOOT_MAX_PARTS 2
static int partcount;

struct rf24boot_partition *parts[CONFIG_BOOT_MAX_PARTS];

void rf24boot_add_part(struct rf24boot_partition *part)
{
	dbg("Registering partition %d: %s\n", partcount, part->info.name);
	BUG_ON(partcount >= CONFIG_BOOT_MAX_PARTS); /* Increase max parts */
	parts[partcount++] = part;
}

uint8_t  local_addr[5] = { 
	CONFIG_RF_ADDR_0, 
	CONFIG_RF_ADDR_1, 
	CONFIG_RF_ADDR_2, 
	CONFIG_RF_ADDR_3, 
	CONFIG_RF_ADDR_4 
};

ANTARES_INIT_HIGH(slave_init) 
{
	dbg("Wireless in slave mode\n");
	rf24_open_reading_pipe(g_radio, 1,  local_addr);
	rf24_start_listening(g_radio);
}

void respond(uint8_t* buffer, uint8_t len)
{
	int ret;
	int retry = 50;
	dbg("respond with %d bytes\n", len);
	do {
		ret = rf24_write(g_radio, buffer, len);
	} while ((ret != 0) && retry--);
	if (!retry)
		printk("Timeout!\n");
}


static uint32_t prev_addr=~0;
void handle_cmd(struct rf24boot_cmd *cmd) {
	if (cmd->op == RF_OP_HELLO)
	{
		struct rf24boot_hello_resp resp;
		resp.numparts = partcount;
		resp.is_big_endian = 0; /* FixMe: ... */
		strncpy(resp.id, CONFIG_SLAVE_ID, 30);
		resp.id[29]=0x0;
		dbg("hello from 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x !\n", 
		    cmd->data[0],
		    cmd->data[1],
		    cmd->data[2],
		    cmd->data[3],
		    cmd->data[4]
			);
		rf24_open_writing_pipe(g_radio, (uint8_t *) cmd->data);
		respond((uint8_t *) &resp, 
			sizeof(struct rf24boot_hello_resp));
	} else if ((cmd->op == RF_OP_PARTINFO) && (cmd->part <= CONFIG_BOOT_MAX_PARTS))
	{
		respond((uint8_t *) &parts[cmd->part]->info, 
			sizeof(struct rf24boot_partition_header));
		
	} else if ((cmd->op == RF_OP_READ) && (cmd->part <= CONFIG_BOOT_MAX_PARTS))
	{
		int ret;
		do {
			ret = parts[cmd->part]->read(parts[cmd->part], cmd);
			respond((uint8_t *) cmd, 
				RF24BOOT_CMDSIZE(parts[cmd->part]->info.iosize));
			cmd->addr += (uint32_t) ret;
		} while (ret);
		cmd->op = RF_OP_NOP;
		respond((uint8_t *) cmd, 
			RF24BOOT_CMDSIZE(0));
	} else if ((cmd->op == RF_OP_WRITE) && (cmd->part <= CONFIG_BOOT_MAX_PARTS))
	{
		if (cmd->addr != prev_addr) /* Discard dupes */
			parts[cmd->part]->write(parts[cmd->part], cmd);
		else
			dbg("Dupe received!\n");
		prev_addr = cmd->addr; /* Store prev. addr */ 
	} else if (cmd->op == 0x10) {
		respond((uint8_t *) cmd, 
			RF24BOOT_CMDSIZE(0));
	} else if ((cmd->op == RF_OP_BOOT) && (cmd->part <= CONFIG_BOOT_MAX_PARTS))
	{
		/* Just boot it! */
		rf24boot_boot_partition(parts[cmd->part]);
	}
}


ANTARES_APP(slave)
{
	struct rf24boot_cmd cmd;
	uint8_t pipe; 
	if ( !rf24_available(g_radio, &pipe)) {
		return;
	}
	
	uint8_t len = rf24_get_dynamic_payload_size(g_radio);
	rf24_read(g_radio, &cmd, len);
	rf24_stop_listening(g_radio);
	handle_cmd(&cmd);
	rf24_start_listening(g_radio);
}
