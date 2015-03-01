#include <librf24/librf24.hpp>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <librf24/rf24boot.hpp>


using namespace rf24boot;
using namespace librf24;

INITIALIZE_EASYLOGGINGPP

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


int main(int argc, const char** argv)
{	
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = new LibRF24LibUSBAdaptor(); 
	
	a->configureFromArgs(argc, argv);

	unsigned char remote_addr[5] = { 0xb0, 0x0b, 0x10, 0xad, 0xed };
	unsigned char  local_addr[5] = { 0xb0, 0x0b, 0xc0, 0xde, 0xed };

	LibRF24PipeOpenTransfer pow(*a, PIPE_WRITE, remote_addr);
	pow.execute(); 

	LibRF24PipeOpenTransfer por(*a, PIPE_READ_0, local_addr);
	por.execute(); 

	rf24bootWaitTarget(a);
	RF24BootPartitionTable ptbl(a, local_addr);
	ptbl.select(1);
//	ptbl.restore("main.cpp");
	ptbl.save("out.bin");
//	ptbl.save("out.bin");

}
