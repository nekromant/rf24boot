#include <arch/antares.h>
#include <arch/delay.h>
#include <rf24boot.h>

#define DEBUG_LEVEL 5
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

void rf24boot_boot_by_name(char* name)
{
	uint8_t i;
	for (i=0; i<partcount; i++)
	{
		if (strcmp(name, parts[i]->info.name)==0)
			rf24boot_boot_partition(parts[i]);
	}

}

static uint8_t got_hello = 0;

uint8_t rf24boot_got_hello()
{
	return got_hello;
}

static uint8_t  local_addr[5] = { 
	CONFIG_RF_ADDR_0, 
	CONFIG_RF_ADDR_1, 
	CONFIG_RF_ADDR_2, 
	CONFIG_RF_ADDR_3, 
	CONFIG_RF_ADDR_4 
};

static uint8_t  remote_addr[5] = { 
	CONFIG_RF_ADDR_0, 
	CONFIG_RF_ADDR_1, 
	CONFIG_RF_ADDR_2, 
	CONFIG_RF_ADDR_3, 
	CONFIG_RF_ADDR_4 
};


ANTARES_INIT_HIGH(slave_init) 
{
	rf24_init(g_radio);
	rf24_enable_dynamic_payloads(g_radio);
	#ifdef CONFIG_RF_RATE_2MBPS
	rf24_set_data_rate(g_radio, RF24_2MBPS);
	#endif
	#ifdef CONFIG_RF_RATE_1MBPS
	rf24_set_data_rate(g_radio, RF24_1MBPS);
	#endif
	#ifdef CONFIG_RF_RATE_250KBPS
	rf24_set_data_rate(g_radio, RF24_250KBPS);
	#endif
	rf24_set_channel(g_radio, CONFIG_RF_CHANNEL);
	info("RF: init done\n");
	info("RF: module is %s P variant\n", rf24_is_p_variant(g_radio) ? "" : "NOT");
	dbg("Wireless in slave mode\n\n");
	rf24_set_retries(g_radio, 15, 15);
	rf24_open_reading_pipe(g_radio, 1,  local_addr);
	rf24_start_listening(g_radio);
	rf24_print_details(g_radio);
}

static uint8_t cont=0; /* continuity counter */

static void respond(uint8_t op, struct rf24boot_cmd *cmd, uint8_t len)
{
	int ret;
	int retry; 

	retry = 100;
	cmd->op = (cont++ << 4) | op;
	do {
		rf24_open_writing_pipe(g_radio, remote_addr);
		ret = rf24_write(g_radio, cmd, len + 1);
		delay_ms(10);
		if (ret==0)
			break;
	} while (--retry);

	printk("resp op %d retry %d\n", op, retry);
	if (!retry) { 
		rf24boot_platform_reset();
	}
}


static inline void handle_cmd(struct rf24boot_cmd *cmd) {
	uint8_t cmdcode = cmd->op & 0x0f;
	if (cmdcode == RF_OP_HELLO)
	{
		dbg("hello from 0x%02x:0x%02x:0x%02x:0x%02x:0x%02x !\n", 
		    cmd->data[0],
		    cmd->data[1],
		    cmd->data[2],
		    cmd->data[3],
		    cmd->data[4]
			);
		cont = 0;
		memcpy(remote_addr, cmd->data, 5);
		struct rf24boot_hello_resp *resp = (struct rf24boot_hello_resp *) cmd->data;		
		resp->numparts = partcount;
		resp->is_big_endian = 0; /* FixMe: ... */
		strncpy(resp->id, CONFIG_SLAVE_ID, 28);
		resp->id[28]=0x0;
		respond(RF_OP_HELLO, cmd, 
			sizeof(struct rf24boot_hello_resp));
		got_hello++;
		return; 
	} 
	
	/* Handle continuity and discard dupes */
	if ((cont & 0xf) != (cmd->op >> 4))
	{
		printk("Dupe: %d %d!\n", cont, (cmd->op >> 4));
		return; /* We got a dupe packet */
	}
	cont++; /* Next one, please */
	
	/* Else we assume everything's fine, nrf24l01 acks, so we can only get dupes */
	struct rf24boot_data *dat = (struct rf24boot_data *) cmd->data;
	/* 
	 * The rest of the commands must have a valid part number
	 * Otherwise - we drop 'em 
	 */

	if (dat->part >= partcount)
		return; 

	if ((cmdcode == RF_OP_PARTINFO))
	{
		memcpy(cmd->data, (uint8_t *) &parts[dat->part]->info, 
		       sizeof(struct rf24boot_partition_header));
		respond(RF_OP_PARTINFO, cmd, sizeof(struct rf24boot_partition_header));		
	} else if ((cmdcode == RF_OP_READ))
	{
		int ret;
		do {
			ret = parts[dat->part]->read(parts[dat->part], dat);
			respond(RF_OP_READ, cmd, 
				ret + 5);
			dat->addr += (uint32_t) ret;	
		} while (ret);
		
	} else if (cmdcode == RF_OP_WRITE)
	{
		parts[dat->part]->write(parts[dat->part], dat);
	} else if ((cmdcode == RF_OP_BOOT))
	{
		rf24boot_boot_partition(parts[dat->part]);
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

	dbg("got cmd: %x \n", cmd.op);
	handle_cmd(&cmd);

	rf24_open_reading_pipe(g_radio, 1,  local_addr);
	rf24_start_listening(g_radio);
}
