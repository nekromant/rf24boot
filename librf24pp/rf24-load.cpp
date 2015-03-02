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

void usage(const char* appname, LibRF24Adaptor *a)
{
	fprintf(stderr,
		"nRF24L01 over-the-air programmer\n"
		"USAGE: %s [options]\n"
		"\t --part=name                  - Select partition\n"
		"\t --file=name                  - Select file\n"
		"\t --write, --read, --verify    - Action to perform (write assumes verify by default)\n"
		"\t --noverify                   - Do not auto-verify when writing\n"
		"\t --local-addr=0a:0b:0c:0d:0e  - Select local addr\n"
		"\t --remote-addr=0a:0b:0c:0d:0e - Select remote addr\n"
		"\t --run[=partid]               - Start code from part partid (default - 0)\n"
		"\t --help - This help\n"
		"Adaptor selection options:\n"
		"\t --adaptor=name               - Use adaptor backend 'name'\n"
		"\t --list-adaptors              - List available 'adaptors'\n"
		, appname);

	a->printAllAdaptorsHelp();

	fprintf(stderr, "\n\n(c) Andrew 'Necromant' Andrianov 2013-2015 <andrew@ncrmnt.org> \n");
	fprintf(stderr, "This is free software, you can use it under the terms of GPLv3 or above\n");
}


void printout(LibRF24Adaptor *a, const unsigned  char *local_addr, const unsigned char *remote_addr)
{
	fprintf(stderr,
		"nRF24L01 over-the-air programmer\n"
		"(c) Necromant 2013-2014 <andrew@ncrmnt.org> \n"
		"This is free software licensed under the terms of GPLv2.\n"
		"Adaptor: %s\n"
		"Local Address:  %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx\n"
		"Remote Address: %.2hhx:%.2hhx:%.2hhx:%.2hhx:%.2hhx\n",
		a->getName(),
		local_addr[0],local_addr[1],local_addr[2],local_addr[3],local_addr[4],
		remote_addr[0],remote_addr[1],remote_addr[2],remote_addr[3],remote_addr[4]
		);
}
int main(int argc, const char** argv)
{	
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = LibRF24Adaptor::fromArgs(argc, argv); 
	a->setConfigFromArgs(argc, argv);

	unsigned char remote_addr[5] = { 0xb0, 0x0b, 0x10, 0xad, 0xed };
	unsigned char  local_addr[5] = { 0xb0, 0x0b, 0xc0, 0xde, 0xed };

	printout(a, local_addr, remote_addr);
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
