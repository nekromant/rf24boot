#include <librf24/librf24.hpp>
#include <iostream>
using namespace librf24;
#include <unistd.h>
#include <getopt.h>

INITIALIZE_EASYLOGGINGPP

int main(int argc, const char** argv)
{
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = LibRF24Adaptor::fromArgs(argc, argv);
	a->setConfigFromArgs(argc, argv);

    struct librf24::rf24_usb_config conf;
    conf.channel          = 13;
    conf.rate             = RF24_2MBPS;
    conf.pa               = RF24_PA_MAX;
    conf.crclen           = RF24_CRC_16;
    conf.num_retries      = 0xf;
    conf.retry_timeout    = 0xf;
    conf.dynamic_payloads = 1;
    conf.payload_size     = 32;
    conf.ack_payloads     = 0;
    conf.pipe_auto_ack    = 0xff;
	a->setConfig(&conf);

	unsigned char addr[5] = { 0x8,  0x5a, 0x76, 0xa6, 0x10 };
	char state;

	optind = 0;
	opterr = 0;
    int c;
    int digit_optin;
    while (1) {
        int this_option_optind = optind ? optind : 1;
        int option_index = 0;
        static struct option long_options[] = {
            {"relay",   required_argument, 0,  'r' },
            {"state",   required_argument, 0,  's' },
            {0,         0,                 0,  0 }
		};
    
        c = getopt_long(argc, argv, "r:s:",
                 long_options, &option_index);
        if (c == -1)
            break;
		switch(c) {
			case 'r':
				addr[4]=atoi(optarg);
				LOG(INFO) << 's';
				break;
			case 's':
				state=atoi(optarg);
				break;

		}
	}
	
	LibRF24Address acl(addr);

	LibRF24PipeOpenTransfer pow(*a, PIPE_WRITE, addr);
	pow.execute();
	
	char data[32];
	data[0]=0xf1;
	data[1]=state ? 0x1 : 0;
	LibRF24IOTransfer t(*a);
	t.makeWriteStream(true);
	t.appendFromBuffer(data, 32);
	t.setTimeout(3000);
	LOG(INFO) << "Sending!";
	t.execute();
	LOG(INFO) << "result: " << t.getLastWriteResult();;
}
