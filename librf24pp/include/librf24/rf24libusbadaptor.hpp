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
		~LibRF24LibUSBAdaptor();

		/*
		virtual bool submit(LibRF24Transfer *t);
		virtual bool cancel(LibRF24Transfer *t);
		virtual void loopOnce();
		virtual void loopForever();
		*/

	protected:
		void requestStatus();
		
	private:

		libusb_context               *ctx;
		libusb_device               **devList = nullptr;
		libusb_device                *thisDevice; 
		libusb_device_handle         *thisHandle; 

		std::vector<struct libusb_transfer *> transferPool;

		struct libusb_transfer       *interruptTransfer;		
		struct rf24_dongle_status     interruptBuffer;
		int cbSlots  = 0;
		int acbSlots = 0;


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

	};

};
