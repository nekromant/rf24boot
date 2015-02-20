#pragma once
#include <vector>
#include <librf24/rf24defs.h>

namespace librf24 {
	class LibRF24Transfer;
	class LibRF24Packet;
	class LibRF24Adaptor {
	protected:
		std::vector<LibRF24Transfer *> queue;

	public:
		LibRF24Adaptor();
		~LibRF24Adaptor();
		uint64_t currentTime();		
		virtual bool submit(LibRF24Transfer *t);
		virtual bool cancel(LibRF24Transfer *t);
		virtual void loopOnce();
		virtual void loopForever();
		virtual std::vector<std::pair<int,short>> getPollFds();
		
	protected:
		int countToWrite = 0; 
		int countToRead  = 0;
		int currentMode  = MODE_IDLE;

		virtual void bufferWrite(LibRF24Packet *pck);
		virtual void bufferRead(LibRF24Packet *pck);
		virtual void requestStatus();

		LibRF24Packet *nextForRead();
		LibRF24Packet *nextForWrite();
		void bufferReadDone(LibRF24Packet *pck);
		void bufferWriteDone(LibRF24Packet *pck);
		void updateStatus(int newMode, int countCanWrite, int countCanRead, int lastResult);
		void startTransfers();

/*
		virtual void configure(struct rf24_usb_config *conf);
		virtual void configDone();

		virtual void sweep();
		virtual void sweepDone();
*/
	private:


	};		
};
