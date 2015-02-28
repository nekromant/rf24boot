#include <librf24/librf24.hpp>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

namespace rf24boot { 
#include "../include/rf24boot.h"
}

using namespace rf24boot;
using namespace librf24;


INITIALIZE_EASYLOGGINGPP

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

static const char prg[] = { 
	'\\',
	'|',
	'/',
	'-'
};


void rf24bootWaitTarget(LibRF24Adaptor *a)
{
	LibRF24IOTransfer ping(*a);
	ping.makeWriteStream(true); 
	char tmp = RF_OP_NOP;
	ping.fromBuffer(PIPE_WRITE, &tmp, 1);
	bool online = false;
	fprintf(stderr, "Waiting for target....");
	int i=0;
	while (!online) { 
		if (i>0)
			usleep(100000);	
		ping.execute();
		fputc(0x08, stderr);
		fputc(prg[i++ & 0x3], stderr);
		online = ping.getLastWriteResult();
		if (online)
			fprintf(stderr, "\x8GOTCHA!\n");
	}
}

void rf24RequestPartitionTable(LibRF24Adaptor *a)
{
	LibRF24IOTransfer rq(*a);
	
}

int main(int argc, const char** argv)
{	
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = new LibRF24LibUSBAdaptor(); 
	
	a->configureFromArgs(argc, argv);

	unsigned char addr[5] = { 0xb0, 0x0b, 0x10, 0xad, 0xed };

	LibRF24PipeOpenTransfer pow(*a, PIPE_WRITE, addr);
	pow.submit(); 

	rf24bootWaitTarget(a);
	rf24RequestPartitionTable(a);

}
