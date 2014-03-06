#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>

#define _GNU_SOURCE
#include <getopt.h>


#define I_VENDOR_NUM        0x1d50
#define I_VENDOR_STRING     "www.ncrmnt.org"
#define I_PRODUCT_NUM       0x6032
#define I_PRODUCT_STRING    "nRF24L01-tool"


#include "requests.h"
#include "rf24.h"
#include "rf24boot.h"

uint8_t local_addr[]  = { 0x0, 0x1, 0x2, 0x3, 0x3 } ;
uint8_t remote_addr[] = { 0x0, 0x1, 0x2, 0x3, 0x4 } ;





static uint8_t cont;

void dump_buffer(char* buf, int len)
{
	int i;
	for (i=0;i<len; i++)
		printf("0x%hhx ", buf[i]);
	printf("\n");
	for (i=0;i<len; i++)
		printf("%c ", buf[i]);

	printf("\n\n");
}

int rf24boot_send_cmd(usb_dev_handle *h, uint8_t opcode, char* data, int size)
{
	struct rf24boot_cmd cmd;
	memset(&cmd, 0x0, sizeof(cmd));
	cmd.op = (cont++ << 4) | opcode;
	memcpy(cmd.data, data, size);
	int ret;
	int retry=1;
	do { 
		ret = rf24_write(h, (char*) &cmd, size + 1);
	} while ((ret != 0) && (--retry));
	if (!retry)
		return -1;
	return ret;
}


int rf24boot_get_cmd(usb_dev_handle *h, char* buf, int len)
{
	struct rf24boot_cmd cmd;
	int ret; 
	int retry=10;
	do { 
		ret = rf24_read(h, &cmd, 32);
		if ( ret <= 0)
			continue;
		/* Check if a dupe! */
		if ((cmd.op >> 4) != (cont & 0xf)) {
			printf("\nGot dupe: %x %x!\n", cmd.op >> 4, cont);
			continue; /* dupe */
		}
		cont++;
		break;
	} while (--retry);
	if (!retry)
		return -1;
	memcpy(buf, cmd.data, len);
	return cmd.op & 0xf;
}


int rf24boot_xfer_cmd(usb_dev_handle *h, 
		      uint8_t opcode, char* data, int len, 
		      char* rbuf, int rlen)
{
	do { 
		rf24_listen(h, 0);
		int ret = rf24boot_send_cmd(h, opcode, data, len);
		if ( ret < 0 ) { 
			//fprintf(stderr, "\ntimeout: Lost ack?\n");
		}
		rf24_listen(h, 1);
		ret = rf24boot_get_cmd(h, rbuf, rlen);
		if (ret < 0) {
			fprintf(stderr, "\ntimeout: no response (interference?)\n");
			continue;
		}
		rf24_listen(h, 0);
		return ret;
	}
	 while (1);
}

int rf24_wait_target(usb_dev_handle *h)
{
	fprintf(stderr, "Waiting for target...");
	int ret;
	do { 
		fflush(stderr);
		fprintf(stderr, ".");
		ret = rf24boot_send_cmd(h, RF_OP_NOP, NULL, 0);
		sleep(1);
	} while (ret!=0);
	fprintf(stderr, "FOUND!\n");
}


void rf24boot_save_partition(usb_dev_handle *h, struct rf24boot_partition_header *hdr, FILE* fd, int pid)
{
	struct rf24boot_data dat; 
	dat.addr = 0;
	dat.part = pid;
	rf24_listen(h, 0);
	rf24boot_send_cmd(h, RF_OP_READ, &dat, sizeof(dat));
	rf24_listen(h, 1);
	int ret;
	int dupes;
	int maxdupes = 0;
	uint32_t prev_addr = 0;
	do { 
		rf24boot_get_cmd(h, &dat, sizeof(dat));
		fwrite(dat.data, 1, hdr->iosize, fd);
		fprintf(stderr, "Reading partition %s: %x %u/%u cont %hhx    \r", 
			hdr->name, dat.addr, dat.addr + hdr->iosize, hdr->size, cont);
		fflush(stderr);
		fflush(fd);
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
			fprintf(stderr, "\n\nDone!\n");
			break;
		}
	
	} while (1);
}

void rf24boot_verify_partition(usb_dev_handle *h, struct rf24boot_partition_header *hdr, FILE* fd, int pid)
{
	struct rf24boot_data dat; 
	dat.addr = 0;
	dat.part = pid;
	rf24_listen(h, 0);
	rf24boot_send_cmd(h, RF_OP_READ, &dat, sizeof(dat));
	rf24_listen(h, 1);
	int ret;
	int dupes;
	int i;
	int maxdupes = 0;
	unsigned char tmp[32];
	uint32_t prev_addr = 0;
	do { 
		rf24boot_get_cmd(h, &dat, sizeof(dat));
		ret = fread(tmp, 1, hdr->iosize, fd);
		fprintf(stderr, "Verifying partition %s: %u/%u \r", 
			hdr->name, dat.addr + hdr->iosize, hdr->size);
		fflush(stderr);
		fflush(fd);
		if (dat.addr && (prev_addr != dat.addr - hdr->iosize))
		{
			fprintf(stderr, "\n\n EPIC FAILURE: Sync lost\n");
			fprintf(stderr, "\n   info: addr 0x%x prev 0x%x expected 0x%x\n", 
				dat.addr, prev_addr, dat.addr - hdr->iosize);
			fprintf(stderr, "\n   This is likely a very nasty bug. Please report it!\n\n", 
				dat.addr, prev_addr, dat.addr - hdr->iosize);

			exit(1);
		}

		prev_addr = dat.addr;
		if ((dat.addr + hdr->iosize >= hdr->size)) {
			fprintf(stderr, "\n\nDone!\n");
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
}





void rf24boot_restore_partition(usb_dev_handle *h, struct rf24boot_partition_header *hdr, FILE* fd, int pid)
{
	int ret, ret2;; 
	char tmp[32];
	struct rf24boot_data dat;
	dat.addr = 0x0;
	dat.part = pid;
	rf24_listen(h, 0);
	do { 
		ret = fread(dat.data, 1, hdr->iosize, fd);
		fprintf(stderr, "Writing partition %s: %u/%u \r", hdr->name, dat.addr + hdr->iosize, hdr->size);
		fflush(stderr);		
		rf24boot_send_cmd(h, RF_OP_WRITE, &dat, sizeof(dat));
		if (ret  != hdr->iosize) {			
			break; /* EOF */
		}
		dat.addr += hdr->iosize;
		if (dat.addr >= hdr->size)
			break;
	} while (1);
	if (!hdr->pad || 0 == (dat.addr % hdr->pad))
		goto done;
	/* Else, we need padding */
	int padsize = hdr->pad - (dat.addr % hdr->pad);
	fprintf(stderr, "\nWill pad %d bytes!\n", padsize);
	memset(dat.data, 0xff, hdr->iosize);
	do { 
		dat.addr += hdr->iosize;
		padsize -= hdr->iosize;
		rf24boot_send_cmd(h, RF_OP_WRITE, &dat, sizeof(dat));
	} while (padsize>hdr->iosize);

done:	/* ToDo: Padding here */
	fprintf(stderr, "\n\nDone!\n");
}



void usage(char *appname)
{
	fprintf(stderr, 
		"nRF24L01 over-the-air programmer\n"
		"USAGE: %s [options]\n"
		"\t --channel=N                  - Use Nth channel instead of default (76)\n"
		"\t --rate-{2m,1m,250k}          - Set data rate\n"
		"\t --pa-{min,low,high,max}      - Set power amplifier level\n"
		"\t --part=name                  - Select partition\n"
		"\t --file=name                  - Select file\n"
		"\t --write, --read              - Select action\n"
		"\t --local-addr=0a:0b:0c:0d:0e  - Select local addr\n"
		"\t --remote-addr=0a:0b:0c:0d:0e - Select remote addr\n"
		"\t --run[=appid]                - Run the said appid\n"
		"\t --help                       - This help\n"
		"\t --sweep=n                    - Sweep the spectrum and dump observed channel usage"
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
	int channel = 76;
	int rate = RF24_2MBPS;
	int pa = RF24_PA_MAX;
	int operation=0;
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
		{"local-addr",      required_argument,  NULL,   'a' },
		{"remote-addr",     required_argument,  NULL,   'b' },
		{"run",        optional_argument,  NULL,   'r' },
		{"sweep",      optional_argument,  NULL,   's' },
		{"help",            no_argument,  NULL,   'h' },
		
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
		case 'h':
			usage(argv[0]);
		}
	};

	fprintf(stderr,
		"nRF24L01 over-the-air programmer\n"
		"(c) Necromant 2013-2014 <andrew@ncrmnt.org> \n\n"
		"Local Address:  %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx\n"
		"Remote Address: %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx\n"
		"Channel:        %d \n"
		"Rate:           %s \n"
		"PA Level:       %s\n\n",
		local_addr[0],local_addr[1],local_addr[2],local_addr[3],local_addr[4],
		remote_addr[0],remote_addr[1],remote_addr[2],remote_addr[3],remote_addr[4],
		channel, rate2string[rate], pa2string[pa]
		);

	usb_dev_handle *h;
	h = nc_usb_open(I_VENDOR_NUM, I_PRODUCT_NUM, I_VENDOR_STRING, I_PRODUCT_STRING, NULL);
	if (!h) {
		fprintf(stderr, "No USB device found ;(\n");
		exit(0);
	}
	usb_set_configuration(h, 1);
	usb_claim_interface(h, 0);

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
			i = rf24_sweep(h, do_sweep, buf, 128);
			if (i!=128)
				return;
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

	rf24_set_local_addr(h, local_addr);
	rf24_set_remote_addr(h, remote_addr);
	rf24_set_config(h, channel, rate, pa);
	rf24_open_pipes(h);

	rf24_listen(h, 0);
	rf24_wait_target(h);

	/* Say hello */

	struct rf24boot_hello_resp hello;
	memset(&hello, 0x0, sizeof(hello));

	cont = -1;
	rf24boot_xfer_cmd(h, RF_OP_HELLO, local_addr, 5, &hello, sizeof(hello));

	fprintf(stderr, "Target: %s\n", hello.id);
	fprintf(stderr, "Endianness: %s\n", hello.is_big_endian ? "BIG" : "little");
	fprintf(stderr, "Number of partitions: %d\n", (int) hello.numparts);

	int i;
	struct rf24boot_partition_header *hdr = malloc(
		hello.numparts * sizeof(struct rf24boot_partition_header));

	int activepart = -1; 	
	
	for (i=0; i< hello.numparts; i++) {
		uint8_t part = i;

		rf24boot_xfer_cmd(h, 
				  RF_OP_PARTINFO, &part, 1, 
				  &hdr[i], sizeof(struct rf24boot_partition_header));

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
		rf24boot_restore_partition(h, &hdr[activepart], fd, activepart);
	} else if (operation == 'r') {
		/* Read */
		rf24boot_save_partition(h, &hdr[activepart], fd, activepart);
	} else {
		/* Verify */
		rf24boot_verify_partition(h, &hdr[activepart], fd, activepart);	
	}
	fclose(fd);

bailout:
	if (runappid >=0)
	{
		struct rf24boot_data dat; 
		dat.part = runappid;
		dat.addr = 0;
		printf("Starting application %d...\n", runappid);
		rf24boot_send_cmd(h, RF_OP_BOOT, &dat, sizeof(dat));
	}
	fprintf(stderr, "Have a nice day!\n");
	exit(0);
		
}
