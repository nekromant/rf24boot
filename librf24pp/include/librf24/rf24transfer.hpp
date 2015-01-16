#include <vector>

#pragma once


namespace librf24 {
	class LibRF24Adaptor;
	#include "librf24/rf24defs.h"

	class LibRF24Transfer {
	protected:
		
	public:
		LibRF24Transfer(LibRF24Adaptor &a);
		~LibRF24Transfer();
	private:
		
		unsigned int timeout_ms = 0;
		LibRF24Adaptor &adaptor;
		void (*cb)(LibRF24Transfer &t, int status) = nullptr;
	};
};



