#include <librf24/rf24adaptor.hpp>

using namespace librf24;

typedef std::vector<std::pair<int,short>> pollfdlist;

std::vector<std::pair<int,short>> LibRF24Adaptor::getPollFds() 
{
	pollfdlist dummy;
	return dummy;
}

LibRF24Adaptor::LibRF24Adaptor() 
{
	dbg.setPrefix("LibRF24Adaptor");
	dbg << dbg.error << "Creating dummy adaptor" << std::endl;
}

LibRF24Adaptor::~LibRF24Adaptor()
{
	
}

