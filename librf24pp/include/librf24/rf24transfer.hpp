#include <vector>
#include <librf24/rf24debug.hpp>
#include <stdint.h>

#pragma once


namespace librf24 {
	class LibRF24Adaptor;
	#include "librf24/rf24defs.h"

	class LibRF24Transfer : public LibRF24Debuggable {
	protected:
		
	public:
		LibRF24Transfer(LibRF24Adaptor &a);
		~LibRF24Transfer();
	private:
		enum rf24_transfer_status currentStatus = TRANSFER_IDLE; 
		unsigned int timeout_ms = 0;
		unsigned int timeout_left = 0;
		uint64_t timeStarted; 
		LibRF24Adaptor &adaptor;
		void (*cb)(LibRF24Transfer &t, int status) = nullptr;
		/* Private API */ 
		void fireCallback(enum rf24_transfer_status newStatus); 
		void checkTransferTimeout(); 
		void submit();
		uint64_t currentTime();
		
	};
};

