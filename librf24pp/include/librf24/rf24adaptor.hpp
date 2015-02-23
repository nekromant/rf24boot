#pragma once
#include <vector>
#include <librf24/rf24defs.h>

namespace librf24 {

	class LibRF24Transfer;
	class LibRF24Packet;
	class LibRF24Adaptor {
	protected:

	public:
		LibRF24Adaptor();
		~LibRF24Adaptor();
		uint64_t currentTime();	

		virtual void configureStart(struct rf24_usb_config *conf);
		virtual void pipeOpenStart(enum rf24_pipe pipe, unsigned char addr[5]);
		virtual void sweepStart(int times);
		
// Convenience sync wrappers!
//		int configure(struct rf24_usb_config *conf);
//		int pipeOpen(enum rf24_pipe pipe, char addr[5]);
//		int write(pipe, data, len);
//              int read(pipe, data, len);

		virtual bool submit(LibRF24Transfer *t);
		virtual bool cancel(LibRF24Transfer *t);
		virtual void loopOnce();
		virtual void loopForever();
		virtual std::vector<std::pair<int,short>> getPollFds();
		
	protected:
		friend class LibRF24Transfer;
		friend class LibRF24IOTransfer;
		friend class LibRF24ConfTransfer;
		friend class LibRF24LibUSBAdaptor;
		std::vector<LibRF24Transfer *> queue;
		LibRF24Transfer *currentTransfer = nullptr;

		virtual void bufferWrite(LibRF24Packet *pck);
		void bufferWriteDone(LibRF24Packet *pck);

		virtual void bufferRead(LibRF24Packet *pck);
		void bufferReadDone(LibRF24Packet *pck);

		virtual void requestStatus();
		void updateStatus(int countCanWrite, int countCanRead);
		void updateIdleStatus(bool lastOk);

		void configureDone();
		void pipeOpenDone();

		virtual void switchMode(enum rf24_mode mode);
		void switchModeDone(enum rf24_mode newMode);

		void sweepDone(unsigned char results[128]);
		
		LibRF24Packet *nextForRead();
		LibRF24Packet *nextForWrite();

		void startTransfers();
		void transferCompleted(LibRF24Transfer *t);

/*



		virtual void sweep();
		virtual void sweepDone();
*/
	private:
		int numIosPending=0; /* Current async IOS pending */
		int countToWrite = 0; 
		int countToRead  = 0;
		bool canXfer = true;
		enum rf24_mode currentMode  = MODE_IDLE;


	};		
};
