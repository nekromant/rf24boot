#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "librf24.h"

#define COMPONENT "rf24_listen"
#include "dbg.h"

static int completed = 0;


int hexdump_packet(struct rf24_packet *p)
{
	int ii;
	char ascii[33];
	ascii[32]=0x0;
	printf("P%d len %d| ", p->pipe, p->datalen);
	for (ii=0; ii < 32; ii++)
	{
		printf("%02X ", p->data[ii]);
		if ((p->data[ii] >= 0x20) && (p->data[ii] < 0x7f))
		{
			ascii[ii]	=	p->data[ii];
		}
		else
			ascii[ii]	=	'.';		
	}
	
	printf(" | %s\n", ascii);
}

void write_done_cb(struct rf24_transfer *t)
{
	dbg("Write done\n");
	exit(1);
}

void config_done_cb(struct rf24_transfer *t)
{
	dbg("Dongle config done with status %d\n", t->status);
	completed++;
}

void wait(struct rf24_adaptor *a)
{
	while (!completed)
		rf24_handle_events(a);
	completed = 0;
}


const char addr[] = { 0xb0, 0x0b, 0x10, 0xad, 0xed }; 

int main(int argc, char *argv[])
{
	int ret;
	struct rf24_adaptor *a = rf24_get_adaptor_by_name("vusb-async");
	if (!a)
		err("adaptor not found");
	ret = rf24_init(a, argc, argv);
	if (ret)
		exit(1);


	struct rf24_usb_config conf;
	conf.channel          = 13;
	conf.rate             = RF24_2MBPS;
	conf.pa               = RF24_PA_MAX;
	conf.crclen           = RF24_CRC_16;
	conf.num_retries      = 15;
	conf.retry_timeout    = 15;
	conf.dynamic_payloads = 0xff;
	conf.payload_size     = 32;
	conf.ack_payloads     = 0;
	conf.pipe_auto_ack    = 0xff;


	struct rf24_transfer *t = rf24_allocate_config_transfer(a, &conf);
	rf24_set_transfer_callback(t, config_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t);
	wait(a);

	t = rf24_allocate_openpipe_transfer(a, PIPE_WRITE, addr);
	rf24_set_transfer_callback(t, config_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t);
	wait(a);


	t = rf24_allocate_modeswitch_transfer(a, RF24_MODE_WRITE_BULK);
	rf24_set_transfer_callback(t, config_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t);
	wait(a);


	int i;
	char message[32];
	memset(message, 0x0, 32);
	t = rf24_allocate_io_transfer(a, 8);
	rf24_make_write_transfer(t, RF24_MODE_WRITE_STREAM);
	t->flags = RF24_TRANSFER_FLAG_SYNC;
	rf24_set_transfer_timeout(t, 30000);
	rf24_set_transfer_callback(t, write_done_cb);
	printf("ret = %d\n", rf24_add_packet(t, message, 32, 0));


	rf24_submit_transfer(t);
	
	

	while(1)
		rf24_handle_events(a);

	return 0;
}
