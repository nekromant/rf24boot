#include <librf24/librf24.hpp>

using namespace librf24;


INITIALIZE_EASYLOGGINGPP

int main(int argc, const char** argv)
{	
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = LibRF24Adaptor::fromArgs(argc, argv); 
	a->setConfigFromArgs(argc, argv);
	
//	unsigned char addr[5] = { 0xf6, 0xdc, 0x6a, 0xdd, 0xae };
	unsigned char addr[5] = { 0x8,  0x5a, 0x76, 0xa6, 0x9 };

	LibRF24PipeOpenTransfer pow(*a, PIPE_WRITE, addr);
	pow.execute(); 

	char data[32];
	std::cin.getline(data, 32);
	LOG(INFO) << data;
	LibRF24IOTransfer t(*a);
	t.makeWriteStream(true);
	t.appendFromBuffer(data, 32);
	t.setTimeout(3000);
	LOG(INFO) << "Sending!\n";
	t.execute();
	LOG(INFO) << "result: " << t.getLastWriteResult();;
}
