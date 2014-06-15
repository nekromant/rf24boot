#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <ctype.h>


#define _GNU_SOURCE
#include <getopt.h>




#include "requests.h"
#include "adaptor.h"
#include "rf24boot.h"

char local_addr[]  = { 0x0, 0x1, 0x2, 0x3, 0x3 } ;
char remote_addr[] = { 0xb0, 0x0b, 0x10, 0xad, 0xed } ;

static int trace = 0;
static struct rf24_adaptor *adaptor;


static void display_progressbar(int pad, int max, int value)
{
	float percent = 100.0 - (float) value * 100.0 / (float) max;
	int cols;
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	cols = w.ws_col;

	int txt = printf("%.02f %% done [", percent);
	int max_bars = cols - txt - 7 - pad;
	int bars = max_bars - (int)(((float) value * max_bars) / (float) max);

	if (max_bars > 0) {
		int i;	
		for (i=0; i<bars; i++)
			printf("#");
		for (i=bars; i<max_bars; i++)
			printf(" ");
		printf("]\r");
	}
	fflush(stdout);
}


void dump_buffer(char* buf, int len)
{
	int i;
	for (i=0;i<len; i++)
		printf("0x%hhx ", buf[i]);
	printf("\n");
	for (i=0;i<len; i++)
		if (isalnum(buf[i]))
			fprintf(stderr, "%c ", buf[i]);
		else
			fprintf(stderr, ". ");			
	
	printf("\n\n");
}

int rf24boot_send_cmd(uint8_t opcode, void* data, int size)
{
	struct rf24boot_cmd cmd;
	TRACE("opcode %d len %d\n", opcode, size);
	memset(&cmd, 0x0, sizeof(cmd));
	cmd.op = opcode;
	memcpy(cmd.data, data, size);
	int ret;
	int retry = 5; 

	do { 
	ret = rf24_write(adaptor, (unsigned char*) &cmd, size + 1);

	if (ret == -1) /* Remove failed payload from FIFO */
		rf24_flush(adaptor);
	if (ret == -EAGAIN) /* Remove failed payload from FIFO */
		usleep(10000);

	TRACE("write ret %d\n", ret);
		if (trace) { 
			fprintf(stderr, "-- sent -- \n");
			dump_buffer((char*) &cmd, sizeof(cmd));
			fprintf(stderr, "\n");
		}

	} while ((ret == -EAGAIN) || (ret==-1 && retry--));
	
	return ret;
}

int rf24boot_get_cmd(void* buf, int len)
{
	struct rf24boot_cmd cmd;
	int ret;
	int pipe; 
	int retry=200;
	do { 
		uint32_t delay = 50000;
		memset(&cmd, 0x0, sizeof(cmd));
		ret = rf24_read(adaptor, &pipe, &cmd, 32);
		TRACE("read ret %d from pipe %d\n", ret, pipe);
		if ( ret <= 0) {
			usleep(delay);
			continue;
		}
		if (trace) { 
			fprintf(stderr, "-- Received-- \n");
			dump_buffer((char*) &cmd, sizeof(cmd));
			fprintf(stderr, "\n");
		}
		break;
	} while (--retry);
	TRACE("result: %s retr %d\n", retry ? "ok" : "fail", retry);
	if (!retry)
		return -1;
	memcpy(buf, cmd.data, len);
	return cmd.op & 0xf;
}


int rf24boot_xfer_cmd(uint8_t opcode, void* data, int len, 
		      void* rbuf, int rlen)
{
	int ret;
	do { 
		TRACE("xfer start \n");
		rf24_mode(adaptor, MODE_WRITE_SINGLE);
		rf24boot_send_cmd(opcode, data, len);
		rf24_mode(adaptor, MODE_READ);
		ret = rf24boot_get_cmd(rbuf, rlen);
		if (ret < 0) {
			continue;
		}
		TRACE("xfer done\n");
		return ret;
	}
	while (1);
}


static const char prg[] = { 
	'\\',
	'|',
	'/',
	'-'
};

void rf24_wait_target()
{
	int ret; 
	int i = 0;
	rf24_mode(adaptor, MODE_WRITE_SINGLE);
	fprintf(stderr, "Waiting for target....");
	do { 
		ret = rf24boot_send_cmd(RF_OP_NOP, NULL, 0);
		usleep(10000);
		fputc(0x08, stderr);
		fputc(prg[i++], stderr);
		i&=0x3;
	} while (ret<0);
	fprintf(stderr, "\x8GOTCHA!\n");
}


void rf24boot_save_partition(struct rf24boot_partition_header *hdr, FILE* fd, int pid)
{
	struct rf24boot_data dat; 
	dat.addr = 0;
	dat.part = pid;

	rf24_mode(adaptor, MODE_WRITE_SINGLE);
	rf24boot_send_cmd(RF_OP_READ, &dat, sizeof(dat));
	rf24_mode(adaptor, MODE_READ);

	uint32_t prev_addr = 0;
	fprintf(stderr, "Reading partition %s: %u bytes \n", 
		hdr->name, hdr->size);

	do { 
		rf24boot_get_cmd(&dat, sizeof(dat));
		fwrite(dat.data, 1, hdr->iosize, fd);
		if (!trace) { 
			int pad = printf("%u/%u bytes | ", (dat.addr+hdr->iosize), hdr->size); 
			display_progressbar(pad, hdr->size, hdr->size - (dat.addr+hdr->iosize));
		}
		fflush(fd);
		if (dat.addr && (prev_addr != dat.addr - hdr->iosize))
		{
			fprintf(stderr, "\n\n EPIC FAILURE: Sync lost\n");
			fprintf(stderr, "\n   info: addr 0x%x prev 0x%x expected 0x%x\n", 
				dat.addr, prev_addr, dat.addr - hdr->iosize);
			fprintf(stderr, "\n   This is likely a very nasty bug. Please report it!\n\n");
			fprintf(stderr, "\n   And include as much detail as you can!\n\n");

			exit(1);
		}
		prev_addr = dat.addr;
		if ((dat.addr + hdr->iosize >= hdr->size)) {
			fprintf(stderr, "\n\nDone!\n");
			break;
		}
	
	} while (1);
}

void rf24boot_verify_partition(struct rf24boot_partition_header *hdr, FILE* fd, int pid)
{
	struct rf24boot_data dat; 
	dat.addr = 0;
	dat.part = pid;
	int ret;
	int i;
	unsigned char tmp[32];
	uint32_t prev_addr = 0;
	fseek(fd, 0L, SEEK_END);
	long size = ftell(fd);
	rewind(fd);
	printf("Verifying partition %s: %lu/%u bytes \n", 
		hdr->name, size, hdr->size);

	rf24_mode(adaptor, MODE_WRITE_SINGLE);
	rf24boot_send_cmd(RF_OP_READ, &dat, sizeof(dat));
	rf24_mode(adaptor, MODE_READ);

	do { 
		
		ret = rf24boot_get_cmd(&dat, sizeof(dat));
		if (ret <=0 ) {
			continue;
		};
		ret = fread(tmp, 1, hdr->iosize, fd);
		if (!trace) { 
			int pad = printf("%u/%lu bytes | ", (dat.addr), size); 
			display_progressbar(pad, hdr->size, hdr->size - (dat.addr + hdr->iosize));
		}

		if (dat.addr && (prev_addr != dat.addr - hdr->iosize))
		{
			fprintf(stderr, "\n\n EPIC FAILURE: Sync lost\n");
			fprintf(stderr, "\n   info: addr 0x%x prev 0x%x expected 0x%x\n", 
				dat.addr, prev_addr, dat.addr - hdr->iosize);
			fprintf(stderr, "\n   This is likely a very nasty bug. Please report it!\n\n");

			exit(1);
		}

		prev_addr = dat.addr;
		if ((dat.addr + hdr->iosize >= hdr->size)) {
			break;
		}
		for (i=0; i<ret; i++) {
			if (tmp[i] != dat.data[i])
			{
				fprintf(stderr, "Mismatch at address 0x%.02x: 0x%.02hhx != 0x%.02hhx  \n", 
					dat.addr + i, tmp[i], dat.data[i]);
			}
		}
	} while (1);
	printf("\n");
}



void rf24boot_restore_partition(struct rf24boot_partition_header *hdr, FILE* fd, int pid)
{
	int ret; 
	struct rf24boot_data dat;
	dat.addr = 0x0;
	dat.part = pid;
	fseek(fd, 0L, SEEK_END);
	long size = ftell(fd);
	rewind(fd);
	rf24_mode(adaptor, MODE_WRITE_BULK); /* Bulk mode */
	fprintf(stderr, "Writing partition %s: %lu/%u bytes \n", hdr->name, size, hdr->size);
	do { 
		ret = fread(dat.data, 1, hdr->iosize, fd);

		if (!trace) { 
			int pad = printf("%u/%lu bytes | ", (dat.addr), size); 
			display_progressbar(pad, size, size - (dat.addr));
		}
		
		if (ret==0) 
			break;				

		fflush(stderr);	

		rf24boot_send_cmd(RF_OP_WRITE, &dat, sizeof(dat));

		if (dat.addr >= hdr->size)
			break;

		dat.addr += hdr->iosize;

	} while (1);


	if (!hdr->pad || 0 == (dat.addr % hdr->pad))
		goto done;

	/* Else, we need padding */
	int padsize = hdr->pad - (dat.addr % hdr->pad);
	memset(dat.data, 0xff, hdr->iosize);
	do { 
		dat.addr += hdr->iosize;
		padsize -= hdr->iosize;
		rf24boot_send_cmd(RF_OP_WRITE, &dat, sizeof(dat));
	} while (padsize>hdr->iosize);

done: 
	ret = rf24_sync(adaptor, 1500); /* Wait for stuff to fly out */
	if (!ret) {
		fprintf(stderr, "Failed to write all bytes in a bulk transaction\n");
		fprintf(stderr, "Bad signal, interference or problems with the hardware\n");
		exit(1);
	}
	rf24_mode(adaptor, MODE_READ); /* Disable stream mode */
	printf("\n");
}



void usage(char *appname)
{
	fprintf(stderr, 
		"nRF24L01 over-the-air programmer\n"
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

char *rate2string[] = {
	"1MBPS",
	"2MBPS",
	"250KBBPS"
};

char *pa2string[] = {
	"Min",
	"Low",
	"High",
	"Max"
};

int main(int argc, char* argv[])
{
	int channel = 13;
	int rate = RF24_2MBPS;
	int pa = RF24_PA_MAX;
	int operation=0;
	int verifywrite=1; /* FixMe: Autovfy */
	char *filename = NULL; 
	char* partname = NULL;
	const char* short_options = "c:pf:a:b:hr:s:";
	
	const struct option long_options[] = {
		{"channel",   required_argument,  NULL,        'c'          },
		{"pa-min",    no_argument,        &pa,         RF24_PA_MIN  },
		{"pa-low",    no_argument,        &pa,         RF24_PA_LOW  },
		{"pa-high",   no_argument,        &pa,         RF24_PA_HIGH },
		{"pa-max",    no_argument,        &pa,         RF24_PA_MAX  },
		{"rate-2m",   no_argument,        &rate,       RF24_2MBPS   },
		{"rate-1m",   no_argument,        &rate,       RF24_1MBPS   },
		{"rate-250k", no_argument,        &rate,       RF24_250KBPS },
		{"part",      required_argument,  NULL,         'p'},
		{"file",      required_argument,  NULL,         'f'},
		{"write",     no_argument,        &operation,   'w' },
		{"read",      no_argument,        &operation,   'r' },
		{"verify",    no_argument,        &operation,   'v' },
		{"noverify",  no_argument,        &verifywrite,  0 },
		{"trace",     no_argument,        &trace,        1 },
		{"local-addr",      required_argument,  NULL,   'a' },
		{"remote-addr",     required_argument,  NULL,   'b' },
		{"run",        optional_argument,  NULL,   'r' },
		{"sweep",      optional_argument,  NULL,   's' },
		{"help",            no_argument,  NULL,   'h' },
		{"list-adaptors",            no_argument,  NULL,   'l' },		
		{"adaptor",            required_argument,  NULL,   'd' },		
		{NULL, 0, NULL, 0}
	};
	int rez;
	int runappid = -1;
	int do_sweep = 0;
	while ((rez=getopt_long(argc, argv, short_options,
				long_options, NULL))!=-1)
	{
		switch(rez) {
		case 'c':
			channel = atoi(optarg);
			break;
		case 'f':
			filename = optarg;
			break;
		case 'a':
			sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx", 
			       &local_addr[0],&local_addr[1],&local_addr[2],&local_addr[3],&local_addr[4]);
			break;
		case 'b':
			sscanf(optarg, "%hhx:%hhx:%hhx:%hhx:%hhx", 
			       &remote_addr[0],&remote_addr[1],&remote_addr[2],&remote_addr[3],&remote_addr[4]);
			break;
		case 'p':
			partname = optarg;
			break;
		case 'r':
			if (optarg)
				runappid = atoi(optarg);
			else
				runappid = 0;
			break;
		case 's':
			if (optarg)
				do_sweep = atoi(optarg);
			else
				do_sweep = 1;
			break;
		case 'l': 
		{
			rf24_list_adaptors();
			exit(1);
			break;
		}
		case 'd': 
			adaptor = rf24_get_adaptor_by_name(optarg);
			if (!adaptor) { 
				fprintf(stderr, "No such adaptor: %s\n", optarg);
				fprintf(stderr, "Try %s --list-adaptors\n", argv[1]);
				exit(1);
			};
			
			break;
		case 'h':
			usage(argv[0]);			
		}
	};

	if (adaptor == NULL)
		adaptor = rf24_get_default_adaptor();
	
	adaptor->debug = trace;
	fprintf(stderr,
		"nRF24L01 over-the-air programmer\n"
		"(c) Necromant 2013-2014 <andrew@ncrmnt.org> \n"
		"This is free software licensed under the terms of GPLv2.\n"
		"Adaptor:        %s\n"
		"Local Address:  %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx\n"
		"Remote Address: %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx\n"
		"Channel:        %d \n"
		"Rate:           %s \n"
		"PA Level:       %s\n\n",
		adaptor->name,
		local_addr[0],local_addr[1],local_addr[2],local_addr[3],local_addr[4],
		remote_addr[0],remote_addr[1],remote_addr[2],remote_addr[3],remote_addr[4],
		channel, rate2string[rate], pa2string[pa]
		);

	if (0!=rf24_init(adaptor, argc, argv)) { 
		fprintf(stderr, "Adaptor '%s' initialization failed, exiting\n", adaptor->name);
		exit(1);
	}


	/* TODO: Move this shit to a totally different utility */
	if (do_sweep) {
		int i;
		unsigned char buf[128];
		unsigned int observed[128];
		memset(observed, 0x0, 128 * sizeof(int));
		FILE *Gplt;
		Gplt = popen("gnuplot","w");
		setlinebuf(Gplt);
		fprintf(Gplt, "set autoscale\n"); 
		fprintf(Gplt, "plot '-' w lp\n"); 
		while (1) {
			memset(buf, 0x0, 128);
			i = rf24_sweep(adaptor, do_sweep, buf, 128);
			if (i!=128)
				break;
			for (i=0; i<128; i++)
				observed[i]+=buf[i];
			
			for (i=0; i<128; i++) {
				fprintf(Gplt, "%d %d\n",i, observed[i]);
			}
			fprintf(Gplt,"e\n");
			fprintf(Gplt,"replot\n");
			fflush(stdout);
		}
		exit(0);
	}

	rf24_open_reading_pipe(adaptor, 1, local_addr);
	rf24_open_writing_pipe(adaptor, remote_addr);

	struct rf24_config conf; 
	conf.channel          = channel;
	conf.rate             = rate;
	conf.pa               = pa; 
	conf.crclen           = RF24_CRC_16;
	conf.num_retries      = 15; 
	conf.retry_timeout    = 10;
	conf.dynamic_payloads = 1;
	conf.ack_payloads     = 0;
	rf24_config(adaptor, &conf); 

	rf24_wait_target();
	/* Say hello */
	
	struct rf24boot_hello_resp hello;
	memset(&hello, 0x0, sizeof(hello));

	rf24boot_xfer_cmd(RF_OP_HELLO, local_addr, 5, &hello, sizeof(hello));

	fprintf(stderr, "Target:     %s\n", hello.id);
	fprintf(stderr, "Endianness: %s\n", hello.is_big_endian ? "BIG" : "little");
	fprintf(stderr, "Partitions: %d\n", (int) hello.numparts);


	int i;
	struct rf24boot_partition_header *hdr = malloc(
		hello.numparts * sizeof(struct rf24boot_partition_header));

	
	int activepart = -1; 	
	
	for (i=0; i< hello.numparts; i++) {
		rf24boot_get_cmd(&hdr[i], sizeof(struct rf24boot_partition_header));
		fprintf(stderr, "%d. %s size %d iosize %d pad %d\n", 
			i, hdr[i].name, hdr[i].size, hdr[i].iosize, hdr[i].pad );
		if (partname && strcmp(hdr[i].name,partname)==0) 
			activepart = i;
	}
	
	if (!operation)
		goto bailout;
	if (!partname) {
		fprintf(stderr, "No partition specified\n");
		return 1;
	}
	if (activepart == -1) {
		fprintf(stderr, "Partition '%s' not found\n", partname);
		return 1;
	}
	
	if (!filename) {
		fprintf(stderr, "No filename specified\n");
		return 1;		
	}
	FILE* fd = NULL;
	if (strcmp(filename,"-")==0)
	{
		
		 fd = (operation=='w') ? stdin : stdout;
	} else 
	{
		if (operation=='w' || operation == 'v') { 
			fd = fopen(filename, "r");
		} else {
			fd = fopen(filename, "w+");			
		}
	}
		
	if (!fd) {
		fprintf(stderr, "Failed to open file: %s\n", filename);
		perror("Error: ");
		return 1;
	}

	if (operation == 'w')
	{
		/* Write */
		rf24boot_restore_partition(&hdr[activepart], fd, activepart);
		/* BROKEN: TODO: Find out why */
		if (verifywrite) { 
			rf24boot_verify_partition(&hdr[activepart], fd, activepart);
		}
	} else if (operation == 'r') {
		/* Read */
		rf24boot_save_partition(&hdr[activepart], fd, activepart);
	} else {
		/* Verify */
		rf24boot_verify_partition(&hdr[activepart], fd, activepart);	
	}

	fclose(fd);

bailout:
	if (runappid >=0)
	{
		struct rf24boot_data dat; 
		dat.part = runappid;
		dat.addr = 0;
		printf("Starting application %d...\n", runappid);
		rf24boot_send_cmd(RF_OP_BOOT, &dat, sizeof(dat));
	}
	fprintf(stderr, "\n\nWe are done! Have a nice day!\n");
	exit(0);
		
}
