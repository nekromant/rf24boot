#include <arch/antares.h>
#include <arch/delay.h>
#include <rf24boot.h>

#define DEBUG_LEVEL CONFIG_DEBUGGING_VERBOSITY
#define COMPONENT "rf24boot"

#include <lib/printk.h>
#include <lib/RF24.h>
#include <lib/panic.h>
#include <string.h>



static uint8_t  local_addr[5] = { 
	CONFIG_RF_ADDR_0, 
	CONFIG_RF_ADDR_1, 
	CONFIG_RF_ADDR_2, 
	CONFIG_RF_ADDR_3, 
	CONFIG_RF_ADDR_4 
};


/* Place on stack - save RAM */ 
struct rf24_config conf = {
	.channel = CONFIG_RF_CHANNEL,
	.pa = RF24_PA_MAX,
	.rate = RF24_2MBPS,
	.crclen = RF24_CRC_16,
	.dynamic_payloads = 1,
	.num_retries      = 15,
	.retry_timeout    = 15,
	.pipe_auto_ack    = 0xff,
	.payload_size     = 32
};

ANTARES_INIT_HIGH(slave_init) 
{

	rf24_init(g_radio); 
	rf24_flush_rx(g_radio); 
	rf24_flush_tx(g_radio); 
	rf24_config(g_radio, &conf);
	rf24_power_up(g_radio);
	rf24_stop_listening(g_radio);	

	info("addr %x:%x:%x:%x:%x channel %d\n", 
	     local_addr[0],
	     local_addr[1],
	     local_addr[2],
	     local_addr[3],
	     local_addr[4],
	     CONFIG_RF_CHANNEL);

	info("RF: init done\n");
	info("RF: module is %s P variant\n", rf24_is_p_variant(g_radio) ? "" : "NOT");
	dbg("Wireless in slave mode\n\n");
	rf24_open_writing_pipe(g_radio, local_addr);
	rf24_print_details(g_radio);
}

ANTARES_APP(slave)
{
	static int i;
	char hello[32];;
	sprintf(hello, "===> %d", i++);
	rf24_flush_tx(g_radio);
	
	int ret = rf24_write(g_radio, hello, strlen(hello));
	printk("SEND: %d\n", ret);
	delay_s(1);
}
