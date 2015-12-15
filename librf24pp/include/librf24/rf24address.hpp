#pragma once
#include <stdlib.h>
#include <iostream>
#include <librf24/rf24defs.h>
#include <librf24/rf24libusbadaptor.hpp>


namespace librf24 {
	class LibRF24Address { 
	public:
		LibRF24Address(const unsigned char *addr);
		LibRF24Address(const char *addrStr);
		LibRF24Address(std::string addrStr);
		void setLSB(unsigned char lsb);
		LibRF24Address& operator=(const LibRF24Address &rhs);
	private: 
		unsigned char addr[5];
	};
};

