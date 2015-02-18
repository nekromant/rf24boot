#include <librf24/librf24.hpp>

using namespace librf24;


INITIALIZE_EASYLOGGINGPP

int main(int argc, char** argv)
{	
	START_EASYLOGGINGPP(argc, argv);

	LibRF24Adaptor *a = new LibRF24LibUSBAdaptor(); 

//	LibRF24Transfer t(*a); 

//	t.submit();
	a->loopOnce();
	a->loopOnce();
	//a->cancel(&t);
	a->loopForever();

	
}
