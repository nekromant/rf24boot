#include <librf24/librf24.hpp>

using namespace librf24;


INITIALIZE_EASYLOGGINGPP

static	struct rf24_usb_config conf;


int main(int argc, const char** argv)
{	
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = new LibRF24LibUSBAdaptor(); 

	a->setConfigFromArgs(argc, argv);

	unsigned char addr[5] = { 0xb0, 0x0b, 0x10, 0xad, 0xed };
	unsigned char addr2[5] = { 0xed, 0xad, 0x10, 0x0b, 0xb0 };

	LibRF24ConfTransfer ct(*a, &conf);
	ct.submit();

	LibRF24PipeOpenTransfer pow(*a, PIPE_WRITE, addr);
	pow.submit(); 

#if 0
	LibRF24IOTransfer t(*a);
	t.makeRead(15);
#else

	LibRF24IOTransfer t(*a);
	std::string hl("sdfvsdfv");
	t.makeWriteBulk(true);
	t.appendFromString(PIPE_WRITE, hl);
#endif

	t.setTimeout(30000);
	t.submit();	

//	while (1) { 
		while (t.status() != TRANSFER_COMPLETED)
			a->loopOnce();
//	}
}
