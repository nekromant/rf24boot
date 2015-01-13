#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <getopt.h>

#include "librf24.h"

#define COMPONENT "rf24_loader"
#include "dbg.h"

static int completed = 0;
static const char addr[6][5]; 
static int open_pipes[6]; 

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



void usage(char *appname)
{
	fprintf(stderr, 
		"nRF24L01-based over-the-air programming tool\n"
		"USAGE: %s [options]\n"
		"\t --adaptor=name               - Use adaptor 'name'\n"
		"\t --list-adaptors              - List available 'adaptors'\n"
		"\t --channel=N                  - Use Nth channel instead of default (76)\n"
		"\t --rate-{2m,1m,250k}          - Set data rate\n"
		"\t --pa-{min,low,high,max}      - Set power amplifier level\n"
		"\t --part=name                  - Select partition\n"
		"\t --file=name                  - Select file\n"
		"\t --write, --read, --verify    - Action to perform\n"
		"\t --noverify                   - Do not auto-verify when writing\n"
		"\t --local-addr=0a:0b:0c:0d:0e  - Select local addr\n"
		"\t --remote-addr=0a:0b:0c:0d:0e - Select remote addr\n"
		"\t --run[=appid]                - Run the said appid\n"
		"\t --help                       - This help\n"
		"\t --sweep=n                    - Sweep the spectrum and dump observed channel usage"
		"\t --trace                      - Enable protocol tracing\n"		
		"\n(c) Necromant 2013-2014 <andrew@ncrmnt.org> \n"
		,appname);
	exit(1);
}


int main(int argc, char *argv[])
{
	int ret;
	int rez;
	int i;
	printf("%ld \n", get_timestamp());
	usleep(4000);
	printf("%ld \n", get_timestamp());
	printf("%ld \n", get_timestamp());
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
		{"help",        no_argument,  NULL,         'h'    },		
		{NULL, 0, NULL, 0}
	};

	const char* short_options = "p:a:h";
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
		case 'h':
			usage(argv[0]);
		default:
			break; 
		}
	}




	while(1)
		rf24_handle_events(a);

	return 0;
}
