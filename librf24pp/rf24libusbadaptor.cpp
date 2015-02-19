#include <librf24/rf24libusbadaptor.hpp>
#include <unistd.h>

/* TODO: Move these somewhere else */
#define I_VENDOR_NUM            0x1d50
#define I_VENDOR_STRING         "www.ncrmnt.org"
#define I_PRODUCT_NUM           0x6032
#define I_PRODUCT_STRING        "nRF24L01-tool"

/* If we're in bootloader mode */
#define I_BOOTVENDOR_NUM        0x16c0
#define I_BOOTVENDOR_STRING     "www.ncrmnt.org"
#define I_BOOTPRODUCT_NUM       0x05df
#define I_BOOTPRODUCT_STRING    "Necromant's uISP"
#define I_BOOTSERIAL_STRING     "USB-NRF24L01-boot"

#define USB_HID_REPORT_TYPE_INPUT   1
#define USB_HID_REPORT_TYPE_OUTPUT  2
#define USB_HID_REPORT_TYPE_FEATURE 3
#define USBRQ_HID_SET_REPORT        0x09


using namespace librf24;

#define NOTNULL(_s) ((_s != NULL) ? _s : "null")

bool LibRF24LibUSBAdaptor::matchString(libusb_device_handle *dev, int index, const char* string) 
{
	unsigned char tmp[256];
	libusb_get_string_descriptor_ascii(dev, index, tmp, 256);
	LOG(DEBUG) << "idx " << index << " str " << tmp << " vs " << NOTNULL(string) ;

	if (string == NULL)
		return true; /* NULL matches anything */

	return (strcmp(string, (char*) tmp) == 0);
}

struct libusb_device *LibRF24LibUSBAdaptor::findBootDongle(const char* serial)
{
	return findDevice(I_BOOTVENDOR_NUM, I_BOOTPRODUCT_NUM, 
			  I_BOOTVENDOR_STRING, I_BOOTPRODUCT_STRING, serial);
	
}

struct libusb_device *LibRF24LibUSBAdaptor::findDongle(const char* serial)
{
	return findDevice(I_VENDOR_NUM, I_PRODUCT_NUM, 
			  I_VENDOR_STRING, I_PRODUCT_STRING, serial);
}


typedef struct deviceInfo {
	char reportId;
	char pageSize[2];
	char flashSize[4];
} __attribute__((packed)) deviceInfo_t;


void LibRF24LibUSBAdaptor::bootDongle(struct libusb_device *dev)
{
	LOG(INFO) << "Dongle in bootloader mode, booting";
	libusb_device_handle *handle;
	
	deviceInfo_t inf;
	inf.reportId = 1;
	char *buffer = (char *) &inf;

	int err = libusb_open(dev, &handle);
	if (err) 
		return; /* TODO: error handling */
		
	
	libusb_control_transfer(handle,
				LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE
				| LIBUSB_ENDPOINT_OUT, USBRQ_HID_SET_REPORT,
				(USB_HID_REPORT_TYPE_FEATURE << 8 | buffer[0]), 0, 
				(unsigned char*) buffer, sizeof(inf),
				5000);
	/* ignore the error here, close the device */
	libusb_close(handle);
	sleep(3);
}


struct libusb_device *LibRF24LibUSBAdaptor::getDongle(const char* serial)
{
	/* First try a booted dongle */
	LOG(DEBUG) << "Searching for a booted dongle";
	struct libusb_device *dev  = findDongle(serial);
	if (dev != nullptr)
		return dev;

	LOG(DEBUG) << "Searching for an unbooted dongle with serial";
	/* Next a 'cold' dongle with specified serial */
	dev = findBootDongle(serial);
	if (dev != nullptr) { 
		bootDongle(dev);
		return getDongle(serial);
	}

	LOG(DEBUG) << "Searching for any unbooted dongle any serial";
	/* Revert to any 'cold' dongle */
	dev = findBootDongle(nullptr);
	if (dev != nullptr) { 
		bootDongle(dev);
		return getDongle(serial);
	}
	
	LOG(DEBUG) << "Looks like nothing's found";
	return dev;
}


struct libusb_device *LibRF24LibUSBAdaptor::findDevice(int vendor,int product, 
						       const char *vendor_name, 
						       const char *product_name,
						       const char *serial) 
{
	
	libusb_device **list;
	if (nullptr != devList) { 
		libusb_free_device_list(devList, 1);
	}

	libusb_device *found = nullptr;

	ssize_t cnt = libusb_get_device_list(ctx, &list);
	devList = list;

	ssize_t i = 0;
	int err = 0;

	if (cnt < 0)
		return nullptr;

	for (i = 0; i < cnt; i++) {
		libusb_device *device = list[i];
		struct libusb_device_descriptor desc;
		libusb_device_handle *handle;
		err = libusb_open(device, &handle);
		if (err)
			continue;

		int r = libusb_get_device_descriptor(device, &desc);
		if (r)
			continue;

		if (desc.idVendor == vendor && desc.idProduct == product
		    && matchString(handle, desc.iManufacturer, vendor_name)
		    && matchString(handle, desc.iProduct, product_name)
		    && matchString(handle, desc.iSerialNumber, serial)) {
			found = device;
		}
		libusb_close(handle);
		
		if (found)
			break;	
	}
	return found;
}

/* packetQueued callback. Return transaction to pool or with next packet */
static void packetQueued(struct libusb_transfer *t)
{
	
}

/* Packet queued callback. Return transaction to pool or with next packet */
static void statusUpdateArrived(struct libusb_transfer *t)
{
	LibRF24LibUSBAdaptor *a = (LibRF24LibUSBAdaptor *)t->user_data;

}


void LibRF24LibUSBAdaptor::requestStatus()
{
	libusb_submit_transfer(interruptTransfer);
}

/* Fetch a spare transfer from pool or allocate a new one */
struct libusb_transfer *LibRF24LibUSBAdaptor::getLibusbTransfer()
{
	struct libusb_transfer *tr;
	if (!transferPool.empty()) { 
		tr = transferPool.back();
		transferPool.pop_back();
	} else {
		tr = libusb_alloc_transfer(0);
	}
	return tr;
}



/* Put a libusb transfer back into the pool */
void LibRF24LibUSBAdaptor::putLibusbTransfer(struct libusb_transfer *tr)
{
	transferPool.push_back(tr);
}


LibRF24LibUSBAdaptor::LibRF24LibUSBAdaptor(const char *serial)
{
	
	LOG(DEBUG) << "Looking for dongle with serial '" << NOTNULL(serial) << "'";

	if (0!=libusb_init(&ctx)) {
		throw std::runtime_error("libusb_init() failed!");
	}	

	thisDevice = getDongle(serial);
	if (nullptr == thisDevice)
		throw std::runtime_error("No supported device found");

	int err = libusb_open(thisDevice, &thisHandle);
	if (err != 0) 
		throw std::runtime_error("Failed to open a device found");
	this->interruptTransfer = libusb_alloc_transfer(0);
	libusb_fill_interrupt_transfer(this->interruptTransfer, thisHandle, 0x81,
				       (unsigned char *) &interruptBuffer, sizeof(interruptBuffer), 
				       statusUpdateArrived, this, 10000);

}

LibRF24LibUSBAdaptor::LibRF24LibUSBAdaptor() : LibRF24LibUSBAdaptor(NULL)
{
	
}

LibRF24LibUSBAdaptor::~LibRF24LibUSBAdaptor()
{
	LOG(DEBUG) << "Destroying adaptor";
	if (nullptr != devList) { 
		libusb_free_device_list(devList, 1);
	}

}
