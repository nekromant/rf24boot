#include <librf24/rf24packet.hpp>
#include <librf24/rf24libusbadaptor.hpp>
#include <unistd.h>
#include <assert.h>
#include <getopt.h>

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

const char* LibRF24LibUSBAdaptor::getName()
{
	return adaptorName.c_str();;
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
			unsigned char tmp[256]; 
			libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, tmp, 256);
			adaptorName.append("/");
			adaptorName.append((char *)tmp);
			found = device;
		}
		libusb_close(handle);
		
		if (found)
			break;	
	}
	return found;
}


/* Packet queued callback. Return transaction to pool or with next packet */
void LibRF24LibUSBAdaptor::statusUpdateArrived(struct libusb_transfer *t)
{
	LibRF24LibUSBAdaptor *a = (LibRF24LibUSBAdaptor *)t->user_data;
	struct rf24_dongle_status *status = (struct rf24_dongle_status *) t->buffer;
	if (t->status != LIBUSB_TRANSFER_COMPLETED) { 
		LOG(ERROR) << "Transfer failed: " << t->status;
		throw std::runtime_error("libusb transfer failed!");
	}

#if 1
	LOG(DEBUG) << "size real  : " << t->actual_length;
	LOG(DEBUG) << "size wanted: " << t->length;
	LOG(DEBUG) << "CB : " << (int) status->cb_count  << "/" << (int) status->cb_size;
	LOG(DEBUG) << "ACB: " << (int) status->acb_count << "/" << (int) status->acb_size; 
	LOG(DEBUG) << "LASTFAILED: " << (int) status->last_tx_failed;
	LOG(DEBUG) << "TXEMPTY   : " << (int) status->fifo_is_empty;
#endif	
	a->interruptIsRunning = false;
	int cb_slots = (status->cb_size - status->cb_count);
	int acb_slots = (status->acb_size - status->acb_count);
	int towrite = (a->currentMode == MODE_READ)  ? acb_slots : cb_slots;
	int toread  = (a->currentMode == MODE_READ)  ? status->cb_count  : status->acb_count;

	if (status->fifo_is_empty && a->currentMode == MODE_WRITE_BULK) { 
		status->last_tx_failed = 0;
	}

	if ((status->cb_count == 0) && 
	    (status->acb_count == 0) && 
	    (status->fifo_is_empty)) { 
		a->updateIdleStatus((status->last_tx_failed == 0));
	}

	a->updateStatus(towrite, toread);
}


void LibRF24LibUSBAdaptor::requestStatus()
{
	if (!interruptIsRunning) { 
		interruptIsRunning = true;
		libusb_submit_transfer(interruptTransfer);
	}
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


void LibRF24LibUSBAdaptor::packetWritten(struct libusb_transfer *t)
{
	if (t->status != LIBUSB_TRANSFER_COMPLETED) { 
		LOG(ERROR) << "Transfer failed: " << t->status;
		throw std::runtime_error("libusb transfer failed!");
	}

	LibRF24Packet *pck = (LibRF24Packet *) t->user_data;
	LibRF24LibUSBAdaptor *a = (LibRF24LibUSBAdaptor *) pck->owner;
	assert (a!=nullptr);
	pck->owner = nullptr;
	LOG(DEBUG) << "packetWritten";
	a->bufferWriteDone(pck);
	a->putLibusbTransfer(t);
}

void LibRF24LibUSBAdaptor::packetObtained(struct libusb_transfer *t)
{
	if (t->status != LIBUSB_TRANSFER_COMPLETED) { 
		throw std::runtime_error("libusb transfer failed!");
	}

	LibRF24Packet *pck = (LibRF24Packet *) t->user_data;
	pck->len = t->actual_length - 1;
	pck->pipe = (enum rf24_pipe) (pck->raw_buffer()[8] & 0xf);

	LibRF24LibUSBAdaptor *a = (LibRF24LibUSBAdaptor *) pck->owner;

	/* 
	   HACK: Interrupt transfers poll every ~10ms at low-speed
	   We can get a full fifo again after we've read everything pending and
	   waste time waiting for the next interrupt.
	   Therefore dongle returns a bit whether it has more data available
	   This won't work for write transfers, though.
	   It gives a ~15% bandwidth improvements while reading
	*/

	bool have_moar = (pck->raw_buffer()[8] & (1<<7));
	if ((have_moar) && (1==a->getPendingIos())) /* Is this the last one? */
	{
		a->updateStatus(-1, 1); /* We can read at least one more! */
	}

	pck->owner = nullptr;
	a->bufferReadDone(pck);
	a->putLibusbTransfer(t);
}

void LibRF24LibUSBAdaptor::bufferWrite(LibRF24Packet *pck)
{
	struct libusb_transfer *t = getLibusbTransfer();
	pck->owner = this;
	unsigned char *buffer = (unsigned char *) pck->raw_buffer();
	buffer[8] = (unsigned char) pck->pipe;
	
	libusb_fill_control_setup( buffer,
				   LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
				   | LIBUSB_ENDPOINT_OUT, RQ_WRITE, 
				   0, 0, LIBRF24_MAX_PAYLOAD_LEN + 1);
	
	libusb_fill_control_transfer(t, thisHandle, buffer,
				     packetWritten, (void *) pck, 3000);

	libusb_submit_transfer(t);

}

void LibRF24LibUSBAdaptor::bufferRead(LibRF24Packet *pck)
{
	struct libusb_transfer *t = getLibusbTransfer();
	pck->owner = this;

	unsigned char *buffer = (unsigned char *) pck->raw_buffer();
	buffer[8] = (char) pck->pipe;

	libusb_fill_control_setup( buffer,
				   LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
				   | LIBUSB_ENDPOINT_IN, RQ_READ, 
				   0, 0, LIBRF24_MAX_PAYLOAD_LEN + 1);
	
	libusb_fill_control_transfer(t, thisHandle, buffer,
				     packetObtained, (void *) pck, 10000);

	libusb_submit_transfer(t);
}


void LibRF24LibUSBAdaptor::modeSwitched(struct libusb_transfer *t)
{
	if (t->status != LIBUSB_TRANSFER_COMPLETED) { 
		throw std::runtime_error("libusb transfer failed!");
	}
	LibRF24LibUSBAdaptor *a = (LibRF24LibUSBAdaptor *) t->user_data;
	a->switchModeDone(a->nextMode);
	a->putLibusbTransfer(t);
	LOG(DEBUG) << "Modeswitch done";
}

void LibRF24LibUSBAdaptor::switchMode(enum rf24_mode mode)
{
	LOG(DEBUG) << "ModeSwitch started!";
	LibRF24Adaptor::switchMode(mode);
	struct libusb_transfer *t = getLibusbTransfer();
	nextMode = mode; 
	libusb_fill_control_setup(xferBuff,
				  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
				  | LIBUSB_ENDPOINT_OUT, RQ_MODE, mode, 0, 0);	
	libusb_fill_control_transfer(t, thisHandle, xferBuff,
				     modeSwitched, this, 10000);
	libusb_submit_transfer(t);
}

void LibRF24LibUSBAdaptor::sweepCompleted(struct libusb_transfer *t)
{
	LOG(DEBUG) << "Sweep done";
	if (t->status != LIBUSB_TRANSFER_COMPLETED) { 
		throw std::runtime_error("libusb transfer failed!");
	}
	LibRF24LibUSBAdaptor *a = (LibRF24LibUSBAdaptor *) t->user_data;
	a->sweepDone(&a->xferBuff[8]);
	a->putLibusbTransfer(t);
}

void LibRF24LibUSBAdaptor::sweepStart(int times)
{
	LOG(DEBUG) << "Spectrum Sweep started!";
	struct libusb_transfer *t = getLibusbTransfer();	

	libusb_fill_control_setup(xferBuff,
				  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
				  | LIBUSB_ENDPOINT_IN, RQ_SWEEP, times, 0,
				  128);

	libusb_fill_control_transfer(t, thisHandle, xferBuff,
				     sweepCompleted, this, 10000);
	libusb_submit_transfer(t);
}

void LibRF24LibUSBAdaptor::configureCompleted(struct libusb_transfer *t)
{
	LOG(DEBUG) << "Configure done";
	if (t->status != LIBUSB_TRANSFER_COMPLETED) { 
		throw std::runtime_error("libusb transfer failed!");
	}
	LibRF24LibUSBAdaptor *a = (LibRF24LibUSBAdaptor *) t->user_data;
	a->configureDone();
	a->putLibusbTransfer(t);
}

void LibRF24LibUSBAdaptor::pipeOpenCompleted(struct libusb_transfer *t)
{
	LOG(DEBUG) << "Pipe Opened";
	if (t->status != LIBUSB_TRANSFER_COMPLETED) { 
		throw std::runtime_error("libusb transfer failed!");
	}
	LibRF24LibUSBAdaptor *a = (LibRF24LibUSBAdaptor *) t->user_data;
	a->pipeOpenDone();
	a->putLibusbTransfer(t);	
}

void LibRF24LibUSBAdaptor::pipeOpenStart(enum rf24_pipe pipe, unsigned char addr[5])
{
	LOG(DEBUG) << "Opening pipe #" << pipe;
	LibRF24Adaptor::pipeOpenStart(pipe, addr);
	struct libusb_transfer *t = getLibusbTransfer();	
	libusb_fill_control_setup(xferBuff,
				  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
				  | LIBUSB_ENDPOINT_OUT, RQ_OPEN_PIPE, 0, 0, 6);
	xferBuff[8] = (unsigned char) pipe;
	memcpy(&xferBuff[9], addr, 5);
	libusb_fill_control_transfer(t, thisHandle, xferBuff,
				     pipeOpenCompleted, this, 1000);
	libusb_submit_transfer(t);
}

void LibRF24LibUSBAdaptor::configureStart(struct rf24_usb_config *conf)
{
	LOG(DEBUG) << "Configuring... ";
	LibRF24Adaptor::configureStart(conf);
	struct libusb_transfer *t = getLibusbTransfer();
	memcpy(&xferBuff[8], conf, sizeof(*conf));
	libusb_fill_control_setup(xferBuff,
				  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
				  | LIBUSB_ENDPOINT_OUT, RQ_CONFIG, 0, 0,
				  sizeof(struct rf24_usb_config));
	
	libusb_fill_control_transfer(t, thisHandle, xferBuff,
				     configureCompleted, this, 1000);
	libusb_submit_transfer(t);
}


LibRF24LibUSBAdaptor::LibRF24LibUSBAdaptor(const char *serial) : LibRF24Adaptor()
{
	startWithSerial(serial);
}

void LibRF24LibUSBAdaptor::startWithSerial(const char *serial)
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

	libusb_claim_interface(thisHandle, 0);

	this->interruptTransfer = libusb_alloc_transfer(0);
	libusb_fill_interrupt_transfer(this->interruptTransfer, thisHandle, 0x81,
				       (unsigned char *) &interruptBuffer, sizeof(interruptBuffer), 
				       statusUpdateArrived, this, 10000);	
}

LibRF24LibUSBAdaptor::LibRF24LibUSBAdaptor(int argc, const char **argv)  : LibRF24Adaptor()
{
	int rez;
	int preverr = opterr;
	opterr=0;
	optind = 0;
	const char* serial = nullptr;
	const char* short_options = "";

	const struct option long_options[] = {
		{"adaptor-serial",       required_argument,  NULL,        's'               },
		{NULL, 0, NULL, 0}
	};

	while ((rez=getopt_long(argc, (char* const*) argv, short_options,
				long_options, NULL))!=-1)
	{
		switch (rez) { 
		case 's':
			serial = optarg;
			break;
		default:
			break;
		}
	};
	opterr = preverr;
	startWithSerial(serial);
}

void LibRF24LibUSBAdaptor::printAdaptorHelp()
{
	fprintf(stderr,
		"LibUSB Adaptor parameters (--adaptor-type=libusb):\n"
		"\t --adaptor-serial=blah      - Use adaptor with 'blah' serial. Default - any\n"
		);
}	
void LibRF24LibUSBAdaptor::loopOnce()
{
	libusb_handle_events(ctx);
} 

void LibRF24LibUSBAdaptor::loopForever() 
{
	while (1) { 
		libusb_handle_events(ctx);
	}
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
	/* TODO: Free transfers */

}
