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
		void printAllAdaptorsHelp();

// Convenience sync wrappers!
//		int configure(struct rf24_usb_config *conf);
//		int pipeOpen(enum rf24_pipe pipe, char addr[5]);
//		int write(pipe, data, len);
//              int read(pipe, data, len);

		static LibRF24Adaptor *fromArgs(int argc, const char **argv);
		virtual bool submit(LibRF24Transfer *t);
		virtual bool cancel(LibRF24Transfer *t);
		virtual void loopOnce();
		virtual void loopForever();
		virtual const char* getName() { return nullptr; };
		virtual std::vector<std::pair<int,short>> getPollFds();
		enum rf24_transfer_status setConfigFromArgs(int argc, const char **argv);
		enum rf24_transfer_status setConfig(const struct rf24_usb_config *conf);
		const struct rf24_usb_config *getCurrentConfig();
		
	protected:
		friend class LibRF24Transfer;
		friend class LibRF24IOTransfer;
		friend class LibRF24ConfTransfer;
		friend class LibRF24LibUSBAdaptor;
		friend class LibRF24PipeOpenTransfer;
		friend class LibRF24SweepTransfer;

		std::vector<LibRF24Transfer *> queue;
		LibRF24Transfer *currentTransfer = nullptr;

		virtual void bufferWrite(LibRF24Packet *pck);
		void bufferWriteDone(LibRF24Packet *pck);

		virtual void bufferRead(LibRF24Packet *pck);
		void bufferReadDone(LibRF24Packet *pck);

		virtual void requestStatus();
		void updateStatus(int countCanWrite, int countCanRead);
		void updateIdleStatus(bool lastOk);

		virtual void configureStart(struct rf24_usb_config *conf);
		void configureDone();
		virtual void pipeOpenStart(enum rf24_pipe pipe, unsigned char addr[5]);
		void pipeOpenDone();

		virtual void switchMode(enum rf24_mode mode);
		void switchModeDone(enum rf24_mode newMode);

		virtual void sweepStart(int times);
		void sweepDone(unsigned char results[128]);
		
		LibRF24Packet *nextForRead();
		LibRF24Packet *nextForWrite();

		void startTransfers();
		void transferCompleted(LibRF24Transfer *t);

	private:
		int numIosPending=0; /* Current async IOS pending */
		int countToWrite = 0; 
		int countToRead  = 0;
		bool canXfer = true;
		enum rf24_mode currentMode  = MODE_IDLE;
		struct rf24_usb_config currentConfig;

	};		
};
