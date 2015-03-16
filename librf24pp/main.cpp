#include <librf24/librf24.hpp>

using namespace librf24;


INITIALIZE_EASYLOGGINGPP


int main(int argc, const char** argv)
{	
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = new LibRF24LibUSBAdaptor(); 

	a->setConfigFromArgs(argc, argv);
	std::cerr << *a->getCurrentConfig();

	unsigned char addr[5] = { 0xb0, 0x0b, 0x10, 0xad, 0xed };

	LibRF24PipeOpenTransfer pow(*a, PIPE_WRITE, addr);
	pow.execute(); 


	LibRF24IOTransfer t(*a);
	std::string hl("1234567890qwertyuiopasdfghjklzxcvbnm");
	t.makeWriteBulk(true);
	t.appendFromString(PIPE_WRITE, hl);
	t.setTimeout(30000);
	std::cout << t.execute() << std::endl;	

}
