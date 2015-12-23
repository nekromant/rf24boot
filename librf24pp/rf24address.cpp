#include <librf24/rf24address.hpp>

using namespace librf24;

LibRF24Address& LibRF24Address::operator=(const LibRF24Address &rhs)
{
	if (this == &rhs)      /* Same object? */
		return *this;
	LOG(INFO) << "COPY!";
	memcpy(this->addr, rhs.addr, 5);
	return *this;
}

LibRF24Address::LibRF24Address(const unsigned char *newAddr)
{
	memcpy(addr, newAddr, 5); 
}

LibRF24Address::LibRF24Address(const char *astr)
{
	sscanf(astr, "%hhx:%hhx:%hhx:%hhx:%hhx",
	       &addr[0], &addr[1], &addr[2], &addr[3], &addr[4]);

}
