#pragma once
#include <vector>
#include <stdexcept>
#include <libusb.h>
#include <librf24/easylogging++.hpp>
#include <librf24/rf24adaptor.hpp>

namespace librf24 {
	class LibRF24Transfer;

	class LibRF24LibUSBAdaptor: public LibRF24Adaptor {
		
	public:
		LibRF24LibUSBAdaptor();
		LibRF24LibUSBAdaptor(const char *serial);
		LibRF24LibUSBAdaptor(int argc, const char **argv);
		~LibRF24LibUSBAdaptor();
		void loopForever();
		void loopOnce();
		void configureStart(struct rf24_usb_config *conf);
		void pipeOpenStart(enum rf24_pipe pipe, unsigned char addr[5]);
		void sweepStart(int times);
		static void printAdaptorHelp();
		const char *getName();
	protected:
		void requestStatus();
		void bufferWrite(LibRF24Packet *pck);
		void bufferRead(LibRF24Packet *pck);
		void switchMode(enum rf24_mode mode);
		
	private:

		libusb_context               *ctx;
		libusb_device               **devList = nullptr;
		libusb_device                *thisDevice; 
		libusb_device_handle         *thisHandle; 
		enum rf24_mode                nextMode; 
		std::string adaptorName = "libusb";

		std::vector<struct libusb_transfer *> transferPool;

		struct libusb_transfer       *interruptTransfer;
		bool                          interruptIsRunning = false;
		struct rf24_dongle_status     interruptBuffer;
		unsigned char                 xferBuff[128];
		
		struct libusb_device *findDevice(int vendor,int product, 
						 const char *vendor_name, 
						 const char *product_name,
						 const char *serial);
		bool matchString(libusb_device_handle *dev, 
				 int index, const char* string);
 
		struct libusb_device *findBootDongle(const char* serial);
		struct libusb_device *findDongle(const char* serial);
		void bootDongle(struct libusb_device *dev);
		struct libusb_device *getDongle(const char* serial);
		struct libusb_transfer *getLibusbTransfer();
		void putLibusbTransfer(struct libusb_transfer *tr);

		LibRF24Transfer *currentTransfer = nullptr;

		/* Libusb callbacks are bound to be static */
		static void packetWritten(struct libusb_transfer *t);
		static void statusUpdateArrived(struct libusb_transfer *t);
		static void packetObtained(struct libusb_transfer *t);
		static void modeSwitched(struct libusb_transfer *t);
		static void configureCompleted(struct libusb_transfer *t);
		static void pipeOpenCompleted(struct libusb_transfer *t);
		static void sweepCompleted(struct libusb_transfer *t);
		void startWithSerial(const char *serial);
	};

};
