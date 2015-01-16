#include <librf24/librf24.hpp>

using namespace librf24;
int main()
{
	LibRF24Adaptor a; 
	a.setDebugStream(&std::cout);
	LibRF24Transfer t(a); 

}
