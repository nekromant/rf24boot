#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "librf24.h"


#define COMPONENT "rf24-sweep"
#include "dbg.h"

static FILE *Gplt;
static int completed = 0;
static unsigned int observed[128];

void sweep_done_cb(struct rf24_transfer *t)
{
	printf("Sweep done with status %d times %d\n", t->status, t->sweep.sweeptimes);
	int i;
	for (i=0; i<128; i++)
		observed[i]+=t->sweep.sweepdata[i];

	for (i=0; i<128; i++)
		fprintf(Gplt, "%d %d\n",i, observed[i]);

	fprintf(Gplt,"e\n");
	fprintf(Gplt,"replot\n");
	sleep(1);
	rf24_submit_transfer(t);
}

void config_done_cb(struct rf24_transfer *t)
{
	printf("Config done with status %d\n", t->status);
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
	
	/* Initialize GNU Plot */
	Gplt = popen("gnuplot","w");
	setlinebuf(Gplt);
	fprintf(Gplt, "set autoscale\n"); 
	fprintf(Gplt, "plot '-' w lp\n"); 

	struct rf24_usb_config conf; 
	conf.channel          = 13;
	conf.rate             = RF24_2MBPS;
	conf.pa               = RF24_PA_MAX; 
	conf.crclen           = RF24_CRC_16;
	conf.num_retries      = 15; 
	conf.retry_timeout    = 10;
	conf.dynamic_payloads = 1;
	conf.ack_payloads     = 0;
	conf.pipe_auto_ack    = 0; /* disable */

	struct rf24_transfer *t = rf24_allocate_config_transfer(a, &conf);
	rf24_set_transfer_callback(t, config_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t);
	wait(a);

	t = rf24_allocate_sweep_transfer(a, 15);
	t->flags = 0;
	rf24_set_transfer_callback(t, sweep_done_cb);
	rf24_submit_transfer(t); 

	while(1)
		rf24_handle_events(a);	

	return 0;
}
	
