#include <librf24/librf24.hpp>
#include <sys/ioctl.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

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

void usage(const char* appname)
{
	fprintf(stderr,
		"nRF24L01 over-the-air programmer\n\n"
		"USAGE: %s [options]\n"
		"\t --part=name                  - Select partition\n"
		"\t --file=name                  - Select file\n"
		"\t --write, --read, --verify    - Action to perform (write assumes verify by default)\n"
		"\t --noverify                   - Do not auto-verify when writing\n"
		"\t --local-addr=0a:0b:0c:0d:0e  - Select local addr\n"
		"\t --remote-addr=0a:0b:0c:0d:0e - Select remote addr\n"
		"\t --run[=partid]               - Start code from part partid (default - 0)\n"
		"\t --help                       - Show this help\n"
		, appname);

	LibRF24Adaptor::printAllAdaptorsHelp();

	fprintf(stderr, "\n(c) Andrew 'Necromant' Andrianov 2013-2015 <andrew@ncrmnt.org> \n");
	fprintf(stderr, "This is free software, you can use it under the terms of GPLv2 or above\n");
}


void printout(LibRF24Adaptor *a, const unsigned  char *local_addr, const unsigned char *remote_addr)
{
	fprintf(stderr,
		"nRF24L01 over-the-air programmer\n"
		"(c) Necromant 2013-2014 <andrew@ncrmnt.org> \n"
		"This is free software licensed under the terms of GPLv2 or above\n"
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



	
	unsigned char remote_addr[5] = { 0xb0, 0x0b, 0x10, 0xad, 0xed };
	unsigned char  local_addr[5] = { 0xb0, 0x0b, 0xc0, 0xde, 0xed };


	int rez;
	int  runappid    = -1;
	bool read        = false;
	bool verify      = false;
	bool noverify    = false;
	bool write       = false;
	const char *filename = nullptr;
	const char *partname = nullptr;

	const char* short_options = "p:f:a:b:hr:";
	const struct option long_options[] = {
		{"part", required_argument, NULL, 'p'},
		{"file", required_argument, NULL, 'f'},
		{"write", no_argument, NULL, 'w' },
		{"help", no_argument, NULL, 'h' },
		{"read", no_argument, NULL, 'r' },
		{"verify", no_argument, NULL, 'v' },
		{"noverify", no_argument, NULL, 'x' },
		{"local-addr", required_argument, NULL, 'a' },
		{"remote-addr", required_argument, NULL, 'b' },
		{"run", optional_argument, NULL, 'X' },

		{NULL, 0, NULL, 0}
	};

	optind = 0;
	opterr = 0;
	
	while ((rez=getopt_long(argc, (char* const*) argv, short_options,
				long_options, NULL))!=-1)
	{
		switch(rez) {
		case 'r':
			LOG(INFO) << "read";
			read = true;
			break;
		case 'w':
			write = true;
			verify = true;
			break;
		case 'x':
			noverify = true;
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
		case 'X':
			if (optarg)
				runappid = atoi(optarg);
			else
				runappid = 0;
			break;
		case 'h':
			usage(argv[0]);
			exit(1);
			break;
		}
	};

	LibRF24Adaptor *a = LibRF24Adaptor::fromArgs(argc, argv); 	
	a->setConfigFromArgs(argc, argv);
	printout(a, local_addr, remote_addr);
	/* ToDo: Settings printout */
	LibRF24PipeOpenTransfer pow(*a, PIPE_WRITE, remote_addr);
	pow.execute(); 

	LibRF24PipeOpenTransfer por(*a, PIPE_READ_0, local_addr);
	por.execute(); 

	rf24bootWaitTarget(a);
	RF24BootPartitionTable ptbl(a, local_addr);

	if ((partname != nullptr) && (filename != nullptr)) { 

		try { 
			ptbl.select(partname);
		} catch(...) { 
			ptbl.select(atoi(partname));
		};
		
		if (read) 
			ptbl.save(filename);
		if (write)
			ptbl.restore(filename);
		if (verify && !(noverify))
			ptbl.verify(filename);
	}

	if (runappid != -1) 
		ptbl.run();

}
