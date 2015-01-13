#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>

#include "librf24.h"

#define COMPONENT "rf24_listen"
#include "dbg.h"

static int completed = 0;
static const char addr[6][5]; 
static int open_pipes[6]; 

int packet_hexdump(struct rf24_packet *p);
int (*packet_printfunc)(struct rf24_packet *p) = packet_hexdump;

int packet_hexdump(struct rf24_packet *p)
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

int packet_raw(struct rf24_packet *p)
{
	fwrite(p->data, p->datalen, 1, stdout);
	fflush(stdout);
}

int packet_text(struct rf24_packet *p)
{
	printf(p->data);
	fflush(stdout);
}

void read_done_cb(struct rf24_transfer *t)
{
	int i; 
	if (t->status = RF24_TRANSFER_COMPLETED) {
		for (i=0; i<t->io.num_data_packets; i++)
			packet_printfunc(&t->io.data[i]);
	}
	
	rf24_submit_transfer(t);
}

void op_done_cb(struct rf24_transfer *t)
{
	if (t->status != RF24_TRANSFER_COMPLETED)
	{
		fprintf(stderr, "Unexpected transfer status: %d\n", t->status);
		exit(1);
	}
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
	int rez;
	int i;
	struct rf24_adaptor *a = rf24_get_adaptor_by_name("vusb-async");
	if (!a)
		err("adaptor not found");
	ret = rf24_init(a, argc, argv);
	if (ret)
		exit(1);


	/* Fill in some default parameters */
	struct rf24_usb_config conf;
	conf.channel          = 110;
	conf.rate             = RF24_2MBPS;
	conf.pa               = RF24_PA_MAX;
	conf.crclen           = RF24_CRC_16;
	conf.num_retries      = 15;
	conf.retry_timeout    = 15;
	conf.dynamic_payloads = 0;
	conf.payload_size     = 32;
	conf.ack_payloads     = 0;
	conf.pipe_auto_ack    = 0xff; 

	librf24_config_from_argv(&conf, argc, argv);
	librf24_print_config(&conf, "Wireless config");

	const struct option long_options[] = {
		{"pipe",        required_argument,  NULL,   'p' },
		{"addr",        required_argument,  NULL,   'a' },
		{"output-hex",  no_argument,        &packet_printfunc,   packet_hexdump },
		{"output-raw",  no_argument,  &packet_printfunc,   packet_raw     },		
		{"output-text", no_argument,  &packet_printfunc,   packet_text    },		
		{NULL, 0, NULL, 0}
	};
	const char* short_options = "p:a:";
	int curpipe = 0;

	optind = 1 ;

	while ((rez=getopt_long(argc, argv, short_options,
				long_options, NULL))!=-1)
	{
		switch (rez) { 
		case 'p':
			curpipe = atoi(optarg);
			if ((curpipe > 5) || (curpipe < 0)) 
				curpipe = 0;
			break;
		case 'a':
			librf24_str2addr(addr[curpipe], optarg);
			open_pipes[curpipe]++;
			break;
		default:
			break; 
		}
	}



	struct rf24_transfer *t = rf24_allocate_config_transfer(a, &conf);
	rf24_set_transfer_callback(t, op_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t);
	wait(a);

	/* Open configured pipes */
	for (i=0; i<6; i++) {
		if (open_pipes[i]) {

			if (i < 2)
				fprintf(stderr, "PIPE%d: %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx\n", i, 
				       addr[i][0], addr[i][1], addr[i][2], addr[i][3], addr[i][4] );
			else
				fprintf(stderr, "PIPE%d: \e[1;37m%.2hhx\e[0m:%.2hhx:%.2hhx:%.2hhx:%.2hhx\n", i, 
				       addr[i][0], addr[1][1], addr[1][2], addr[1][3], addr[1][4] );
			
			t = rf24_allocate_openpipe_transfer(a, i, addr[i]);
			rf24_set_transfer_callback(t, op_done_cb);
			rf24_set_transfer_timeout(t, 30000);
			t->flags = RF24_TRANSFER_FLAG_FREE;
			rf24_submit_transfer(t);
			wait(a);
		}
	}


/*
	t = rf24_allocate_modeswitch_transfer(a, RF24_MODE_READ);
	rf24_set_transfer_callback(t, op_done_cb);
	rf24_set_transfer_timeout(t, 30000);
	t->flags = RF24_TRANSFER_FLAG_FREE;
	rf24_submit_transfer(t);
	wait(a);
*/
	t = rf24_allocate_io_transfer(a, 8);
	rf24_make_read_transfer(t, 1);
	t->flags = 0;
	rf24_set_transfer_timeout(t, 30000);
	rf24_set_transfer_callback(t, read_done_cb);
	rf24_submit_transfer(t);

	while(1)
		rf24_handle_events(a);

	return 0;
}
