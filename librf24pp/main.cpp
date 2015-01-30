#include <librf24/librf24.hpp>

using namespace librf24;

void maxdebug(LibRF24Debuggable &a) 
{
	a.setDebugStream(&std::cout);
	a.setDebugLevel(LibRF24DebugStream::debug);
}

int main()
{
	LibRF24Adaptor *a = new LibRF24Adaptor(); 
	maxdebug(*a);
	LibRF24Transfer t(*a); 
	maxdebug(t);
	t.submit();
	a->loopForever();
}
