#include <vector>
#include <stdint.h>
#include <librf24/easylogging++.hpp>
#include <librf24/rf24defs.h>
#pragma once


namespace librf24 {
	class LibRF24Adaptor;

	class LibRF24Transfer {
	protected:
		friend class LibRF24Adaptor;
		bool checkTransferTimeout(bool finalize);
		void fireCallback(enum rf24_transfer_status newStatus); 
		bool locked = false;
	public:
		LibRF24Transfer(LibRF24Adaptor &a);
		~LibRF24Transfer();
		bool submit();

		int timeout_ms = 1500;
		int timeout_ms_left = 0;

	private:
		enum rf24_transfer_status currentStatus = TRANSFER_IDLE; 
		uint64_t when_started;
		uint64_t when_timed_out;
		LibRF24Adaptor &adaptor;
		void (*cb)(LibRF24Transfer &t, int status) = nullptr;
	};
};

