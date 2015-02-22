#include <librf24/librf24.hpp>

using namespace librf24;


INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv)
{	
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = new LibRF24LibUSBAdaptor(); 

	struct rf24_usb_config conf;

	conf.channel          = 76;
	conf.rate             = RF24_2MBPS;
	conf.pa               = RF24_PA_MAX;
	conf.crclen           = RF24_CRC_16;
	conf.num_retries      = 15;
	conf.retry_timeout    = 15;
	conf.dynamic_payloads = 1;
	conf.payload_size     = 32;
	conf.ack_payloads     = 0;
	conf.pipe_auto_ack    = 0xff; 


	LibRF24ConfTransfer ct(*a, &conf);
	ct.submit();

	unsigned char addr[5] = { 0xb0, 0x0b, 0x10, 0xad, 0xed };
	LibRF24PipeOpenTransfer po(*a, PIPE_WRITE, addr);
	po.submit(); 


	LibRF24IOTransfer t(*a);
	t.makeWriteStream(true);
	std::string str = "hello cruel world blah blah";
	t.fromString(str); 
	t.submit();

	
	while (t.status() != TRANSFER_COMPLETED)
		a->loopOnce();

}
