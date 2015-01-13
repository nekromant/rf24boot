#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <stdint.h>
#include "librf24.h"

/* 
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
 */


static const char *rate2string[] = {
	"1MBPS",
	"2MBPS",
	"250KBBPS"
};

static const char *pa2string[] = {
	"Min",
	"Low",
	"High",
	"Max"
};

static const char *crc2string[] = {
	"OFF",
	"8",
	"16",
};

int librf24_config_from_argv(struct rf24_usb_config *conf, int argc, char argv[])
{
	
	int pa = -1; 
	int rate = -1;
	int channel = -1;
	int rez;
	const char* short_options = "c:";
	const struct option long_options[] = {
		{"channel",   required_argument,  NULL,        'c'          },
		{"pa-min",    no_argument,        &pa,         RF24_PA_MIN  },
		{"pa-low",    no_argument,        &pa,         RF24_PA_LOW  },
		{"pa-high",   no_argument,        &pa,         RF24_PA_HIGH },
		{"pa-max",    no_argument,        &pa,         RF24_PA_MAX  },
		{"rate-2m",   no_argument,        &rate,       RF24_2MBPS   },
		{"rate-1m",   no_argument,        &rate,       RF24_1MBPS   },
		{"rate-250k", no_argument,        &rate,       RF24_250KBPS },
		{NULL, 0, NULL, 0}
	};
	int preverr = opterr;
	opterr=0;
	while ((rez=getopt_long(argc, argv, short_options,
				long_options, NULL))!=-1)
	{
		switch (rez) { 
		case 'c':
			channel = atoi(optarg);
			break;
		default:
			break; 
		}
	}

	if (pa != -1)
		conf->pa = pa; 
	if (rate != -1)
		conf->rate = rate;
	if (channel != -1)
		conf->channel = channel;

	opterr = preverr;

	return 0;
} 

static void dump_pipes(const char *prompt, uint8_t pfield)
{
	int i;
	fprintf(stderr, prompt);
	for (i=0; i<6; i++)
	{
		if (pfield & (1<<i))
			fprintf(stderr, "PIPE%d ", i);
	}
	fprintf(stderr, "\n");
}

void librf24_print_config(struct rf24_usb_config *conf, const char *prompt)
{
	fprintf(stderr, "===== %14s =====\n", prompt);
	fprintf(stderr, "Channel:                 %d\n", conf->channel);
	fprintf(stderr, "Number of retries:       %d\n", conf->num_retries);
	fprintf(stderr, "Retry delay:             %d (%d us)\n", conf->num_retries, (1 + conf->num_retries) * 250);
	fprintf(stderr, "Power Amplifier:         %s\n", pa2string[conf->pa]);
	fprintf(stderr, "Data Rate:               %s\n", rate2string[conf->rate]);
	fprintf(stderr, "CRC Length:              %s\n", crc2string[conf->crclen]);
	fprintf(stderr, "Dynamic payloads:        %s\n", conf->dynamic_payloads ? "enabled" : "disabled");
	
	dump_pipes(
		"Auto Ack on pipes:       ", conf->pipe_auto_ack); 
	dump_pipes(
		"Ack Payloads on pipes:   ", conf->ack_payloads); 

	fprintf(stderr, "==========================\n");
}


int librf24_str2addr(char *addr, const char* straddr)
{
	return sscanf(straddr, "%hhx:%hhx:%hhx:%hhx:%hhx", 
		      &addr[0],&addr[1],&addr[2],&addr[3],&addr[4]);
}
