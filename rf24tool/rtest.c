#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "adaptor.h"


#define COMPONENT "rtest"
#include "dbg.h"


static int completed = 0;
void sweep_done_cb(struct rf24_transfer *t)
{
	printf("Sweep done with status %d\n", t->status);
	completed++;
}

void config_done_cb(struct rf24_transfer *t)
{
	printf("Config done with status %d\n", t->status);
	completed++;
}

void open_done_cb(struct rf24_transfer *t)
{
	printf("Open done with status %d\n", t->status);
	completed++;
}

void write_done_cb(struct rf24_transfer *t)
{
	printf("Write done with status %d\n", t->status);
	completed++;
}

void wait(struct rf24_adaptor *a)
{
	while (!completed)
		rf24_handle_events(a);
	completed = 0;
}

int main(int argc, char *argv[])
{
	int ret; 
	struct rf24_adaptor *a = rf24_get_adaptor_by_name("vusb-async");
	if (!a)
		err("adaptor not found");
	ret = rf24_init(a, argc, argv);
	if (ret) 
		exit(1);
	
	struct rf24_transfer *t = rf24_allocate_sweep_transfer(a, 30);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_set_transfer_callback(t, sweep_done_cb);
	rf24_submit_transfer(t); 

	wait(a);

	struct rf24_config conf; 
	conf.channel          = 13;
	conf.rate             = RF24_2MBPS;
	conf.pa               = RF24_PA_MAX; 
	conf.crclen           = RF24_CRC_16;
	conf.num_retries      = 15; 
	conf.retry_timeout    = 10;
	conf.dynamic_payloads = 1;
	conf.ack_payloads     = 0;

	t = rf24_allocate_config_transfer(a, &conf);
	rf24_set_transfer_callback(t, config_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t);
	wait(a);


	char addr[] = { 0xb0, 0x0b, 0x10, 0xad, 0xed };
	t = rf24_allocate_openpipe_transfer(a, PIPE_WRITE, addr);
	rf24_set_transfer_callback(t, open_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t); 
	wait(a);
	
	t = rf24_allocate_openpipe_transfer(a, PIPE_READ_0, addr);
	rf24_set_transfer_callback(t, open_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t); 
	wait(a);


	char data[] = "hello world";
	t = rf24_allocate_io_transfer(a, 1);
	rf24_make_write_transfer(t, MODE_WRITE_STREAM);
	rf24_add_packet(t, data, strlen(data), 0);

	rf24_set_transfer_callback(t, write_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t); 
	wait(a);
	return 0;
}
	
