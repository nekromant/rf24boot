#include <vector>
#include <librf24/rf24debug.hpp>
#include <stdint.h>

#pragma once


namespace librf24 {
	class LibRF24Adaptor;
	#include "librf24/rf24defs.h"

	class LibRF24Transfer : public LibRF24Debuggable {
	protected:
		friend class LibRF24Adaptor;
		bool checkTransferTimeout(bool finalize);
		void fireCallback(enum rf24_transfer_status newStatus); 
		bool locked = false;
	public:
		LibRF24Transfer(LibRF24Adaptor &a);
		~LibRF24Transfer();
		bool submit();
	private:
		enum rf24_transfer_status currentStatus = TRANSFER_IDLE; 
		int timeout_ms = 0;
		int timeout_ms_left = 0;

		uint64_t when_started;
		uint64_t when_timed_out;

		LibRF24Adaptor &adaptor;
		void (*cb)(LibRF24Transfer &t, int status) = nullptr;
	};
};

