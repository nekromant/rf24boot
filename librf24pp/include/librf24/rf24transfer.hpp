#include <vector>
#include <stdint.h>
#include <librf24/easylogging++.hpp>
#include <librf24/rf24defs.h>
#pragma once


namespace librf24 {
	class LibRF24Adaptor;
	class LibRF24Packet;
	class LibRF24Transfer {
	protected:
		friend class LibRF24Adaptor;
		LibRF24Adaptor &adaptor;
		bool checkTransferTimeout(bool finalize);
		void updateStatus(enum rf24_transfer_status newStatus, bool callback);
		virtual void transferStarted() { } ;
		virtual void bufferWriteDone(LibRF24Packet *pck);
		virtual void bufferReadDone(LibRF24Packet *pck);
		virtual LibRF24Packet *nextForRead();
		virtual LibRF24Packet *nextForWrite();
		virtual void adaptorNowIdle(bool lastIsOk);
		virtual void handleData(unsigned char *data, size_t size) { };
		enum rf24_mode transferMode = MODE_ANY;
	public:
		LibRF24Transfer(LibRF24Adaptor &a);
		~LibRF24Transfer();

		enum rf24_transfer_status status();
		virtual bool submit();
		void inline setCallback(void (*c)(LibRF24Transfer &t))  {
			cb = c;
		} ;

		int timeout_ms = 1500;
		int timeout_ms_left = 0;

	private:
		enum rf24_transfer_status currentStatus = TRANSFER_IDLE; 
		uint64_t when_started;
		uint64_t when_timed_out;

		void (*cb)(LibRF24Transfer &t) = nullptr;
	};
};

