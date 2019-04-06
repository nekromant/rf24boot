#include <librf24/librf24.hpp>

using namespace librf24;


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
    conf.num_retries      = 8;
    conf.retry_timeout    = 0xf;
    conf.dynamic_payloads = 1;
    conf.payload_size     = 32;
    conf.ack_payloads     = 0;
    conf.pipe_auto_ack    = 0xff;
	a->setConfig(&conf);

//	unsigned char addr[5] = { 0xf6, 0xdc, 0x6a, 0xdd, 0xae };
	unsigned char addr[5] = { 0x8,  0x5a, 0x76, 0xa6, 0x10 };
//	unsigned char addr[5] = { 0x10,  0xa6, 0x76, 0x5a, 0x8 };
	LibRF24Address acl(addr);

	LibRF24PipeOpenTransfer pow(*a, PIPE_WRITE, addr);
	pow.execute();
	
	char data[32];
	data[0]=0xf1;
	data[1]=(argv[1][0]=='1') ? 0x1 : 0;
	LibRF24IOTransfer t(*a);
	t.makeWriteBulk(true);
	t.appendFromBuffer(data, 32);
	t.setTimeout(3000);
	LOG(INFO) << "Sending!\n";
	t.execute();
	LOG(INFO) << "result: " << t.getLastWriteResult();;
}
