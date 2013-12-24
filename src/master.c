#include <arch/antares.h>
#include <arch/delay.h>
#include <rf24boot.h>

#define COMPONENT "rf24master"

#include <lib/printk.h>
#include <lib/RF24.h>


static char hw_addr[5] = { 0xff, 0xff, 0xff, 0xff, 0xfa };

static struct rf24boot_cmd cmd;


ANTARES_APP(master)
{
	int ret; 
	dbg("Wireless in master mode\n");
	rf24_open_writing_pipe(g_radio, hw_addr);

	printk("Searching target...");
	while (1) { 
		cmd.op = RF_OP_HELLO;
		ret = rf24_write(g_radio, &cmd, sizeof(cmd));
		printk(".");
		if (ret == 0) 
			break; 
		delay_ms(1000);
	}
	while(1) ;;
}
