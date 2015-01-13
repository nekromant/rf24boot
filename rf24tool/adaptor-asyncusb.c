#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libusb.h>
#include <stdint.h>
#include "librf24-private.h"

#define DEBUG_LEVEL 5
#define COMPONENT "asyncusb"
#include "dbg.h"

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

struct rfvusb_adaptor_status {
	uint8_t buf_count;
	uint8_t buf_size;
	uint8_t ack_count;
	uint8_t ack_size;
	uint8_t num_failed;
	uint8_t clear_to_send;
} __attribute__((packed));

struct rf24_vusb_adaptor {
	struct rf24_adaptor a;
	libusb_context *ctx;
	struct libusb_device *dev;
	struct libusb_device_handle *h;
	struct libusb_transfer *itransfer;
	struct libusb_transfer *cbtransfer;
	struct libusb_transfer *acbtransfer;
	struct libusb_transfer *cmdtransfer;
	char ibuf[8];
	uint8_t failcounter; 
	unsigned char xferbuffer[128 + 8]; /* libusb xfer buffer. Worst case - 128 bytes sweep payload + 8 setup */
};


/* convenience functions to open the device */
static void ncusb_print_libusb_transfer(struct libusb_transfer *p_t) {
	if (NULL == p_t) {
		info("Not a libusb_transfer...\n");
	} else {
		info("libusb_transfer structure:\n");
		info("flags =%x \n", p_t->flags);
		info("endpoint=%x \n", p_t->endpoint);
		info("type =%x \n", p_t->type);
		info("timeout =%d \n", p_t->timeout);
// length, and buffer are commands sent to the device
		info("length =%d \n", p_t->length);
		info("actual_length =%d \n", p_t->actual_length);
		info("buffer =%p \n", p_t->buffer);
		info("status =%d \n", p_t->status);

	}
	return;
}

int ncusb_match_string(libusb_device_handle *dev, int index, const char* string) {
	unsigned char tmp[256];
	libusb_get_string_descriptor_ascii(dev, index, tmp, 256);
	dbg("cmp idx %d str %s vs %s\n", index, tmp, string);
	if (string == NULL)
		return 1; /* NULL matches anything */
	return (strcmp(string, (char*) tmp) == 0);
}

struct libusb_device *ncusb_find_dev(struct libusb_context *ctx, int vendor,
                                     int product, const char *vendor_name, const char *product_name,
                                     const char *serial) {
	libusb_device **list;
	libusb_device *found = NULL;

	ssize_t cnt = libusb_get_device_list(ctx, &list);
	ssize_t i = 0;
	int err = 0;

	if (cnt < 0) {
		err( "no usb devices found\n");
		return NULL;
	}

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
		    && ncusb_match_string(handle, desc.iManufacturer, vendor_name)
		    && ncusb_match_string(handle, desc.iProduct, product_name)
		    && ncusb_match_string(handle, desc.iSerialNumber, serial)) {
			found = device;
		}
		libusb_close(handle);

		if (found)
			break;

	}
	return found;
}

static char* usage = "VUSB-based rf24tool adapter has no options so far \n";

static void rfvusb_usage() {
	fprintf(stderr, usage);
}

static int usb_set_report(struct libusb_device_handle *h, int reportType,
                          char *buffer, int len) {
	int bytesSent;

	bytesSent = libusb_control_transfer(h,
					    LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_DEVICE
					    | LIBUSB_ENDPOINT_OUT, USBRQ_HID_SET_REPORT,
					    (reportType << 8 | buffer[0]), 0, (unsigned char*) buffer, len,
					    5000);
	if (bytesSent != len) {
		if (bytesSent < 0)
			/* fprintf(stderr, "Error sending message: %s\n", usb_strerror()); */
			return -EIO;
	}
	return 0;
}


typedef struct deviceInfo {
	char reportId;
	char pageSize[2];
	char flashSize[4];
} __attribute__((packed)) deviceInfo_t;

static void rfvusb_boot_adapter(struct libusb_device_handle *h) {
	deviceInfo_t inf;
	inf.reportId = 1;
	usb_set_report(h, USB_HID_REPORT_TYPE_FEATURE, (char*) &inf, sizeof(inf));
}

static int rfvusb_handle_events(struct rf24_adaptor *s) {
	struct rf24_vusb_adaptor *a = (struct rf24_vusb_adaptor *) s;
	return libusb_handle_events(a->ctx);
}


static void handle_io_transfer_completion(struct libusb_transfer *lt) 
{
	struct rf24_vusb_adaptor *a = lt->user_data;
	struct rf24_packet *p = (struct rf24_packet *) lt->buffer;

	if (lt->actual_length == 0)
		return; 

	/* TODO: Error handling */
	if (a->a.current_transfer->type == RF24_TRANSFER_READ) { 
		p->datalen = (uint8_t) lt->actual_length - 1; 
	} 
	
	librf24_packet_transferred(&a->a, p);
}

static int rfvusb_submit_io_transfer(struct rf24_vusb_adaptor *a, 
				     int isack,
				     int todevice) 
{
	struct libusb_transfer *lt = isack ? a->acbtransfer : a->cbtransfer;
	if (lt->status != LIBUSB_TRANSFER_COMPLETED)
		return 0; 
	
	dbg("submit io todevice:%d isack%d\n", todevice, isack);

	struct rf24_packet *p = librf24_request_next_packet(&a->a, isack);
	if (!p) { 
		dbg("No packet space now\n");
		return 0; 
	}

	if (todevice) {
		libusb_fill_control_setup( (void *) p,
					   LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
					   | LIBUSB_ENDPOINT_OUT, RQ_WRITE, 
					   0, 0, 32 + 1);
		
	} else 
	{	
		libusb_fill_control_setup( (void *) p,
					   LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
					   | LIBUSB_ENDPOINT_IN, RQ_READ, 
					   0, 0, 32 + 1);

	}

	libusb_fill_control_transfer(lt, a->h, (void *) p,
				     handle_io_transfer_completion, a, 10000);
	
	libusb_submit_transfer(lt);
	return 1; /* Submitted one transfer */
}

static void handle_interrupt_transfer(struct libusb_transfer *t) {
	struct rf24_vusb_adaptor *a = t->user_data;
	if (t->status != LIBUSB_TRANSFER_COMPLETED)
		goto bailout;

	struct rfvusb_adaptor_status *status =
		(struct rfvusb_adaptor_status *) t->buffer;
	
	dbg(
		"status: cb %u/%u acb %u/%u fails: %u qempty: %u\n",
		status->buf_count, status->buf_size, status->ack_count, status->ack_size,
		status->num_failed, status->clear_to_send);
	
	int count = status->buf_count;

	if (!status->clear_to_send)
		count++;

	uint8_t fails = (status->num_failed - a->failcounter);
	librf24_report_state(&a->a, count,  fails);
	a->failcounter = status->num_failed;
	
	if (a->a.current_transfer && RF24_TRANSFER_IS_IO(a->a.current_transfer)) {

		dbg("current mode: %d\n", a->a.current_mode);

		/* TODO: Maybe it's a good idea to submit as many transfers as we can ? */

		if (a->a.current_mode == RF24_MODE_READ) { /* Read */

			if (status->buf_count)
				rfvusb_submit_io_transfer(a, 0, 0);

			if (status->ack_count < status->ack_size)
				rfvusb_submit_io_transfer(a, 1, 1);

		} else { /* WRITE */			
			if (status->buf_count < status->buf_size)
				rfvusb_submit_io_transfer(a, 0, 1);

			if (status->ack_count)
				rfvusb_submit_io_transfer(a, 1, 0);
		}
	}
		
bailout:
	libusb_submit_transfer(t);
}

struct {
	int a;
	int b;
} lookup[] = { { LIBUSB_TRANSFER_COMPLETED, RF24_TRANSFER_COMPLETED }, {
		LIBUSB_TRANSFER_ERROR, RF24_TRANSFER_ERROR
	}, {
		LIBUSB_TRANSFER_TIMED_OUT, RF24_TRANSFER_TIMED_OUT
	}, {
		LIBUSB_TRANSFER_CANCELLED, RF24_TRANSFER_CANCELLED
	}, {
		LIBUSB_TRANSFER_STALL, RF24_TRANSFER_ERROR
	}, {
		LIBUSB_TRANSFER_NO_DEVICE, RF24_TRANSFER_ERROR
	}, {
		LIBUSB_TRANSFER_OVERFLOW, RF24_TRANSFER_ERROR
	},
};

static int get_transfer_status(enum libusb_transfer_status s) {
	int i;
	for (i = 0; i < ARRAY_SIZE(lookup); i++)
		if (lookup[i].a == s)
			return lookup[i].b;
	return RF24_TRANSFER_ERROR;
}

/* Simple transfers map into single usb transactions. config, openpipe, etc. */
static void handle_simple_transfer(struct libusb_transfer *lt) 
{
	struct rf24_transfer *t = lt->user_data;
	t->status = get_transfer_status(lt->status);
	if (t->type == RF24_TRANSFER_SWEEP) {
		memcpy(t->sweep.sweepdata, &lt->buffer[8], 128);
	}

	if (RF24_TRANSFER_IS_CONF(t))
		rf24_callback(t);

}


static int rfvusb_init(void *s, int argc, char* argv[]) {
	struct rf24_vusb_adaptor *a = s;
	libusb_init(&a->ctx);
	struct libusb_device *d = ncusb_find_dev(a->ctx, I_BOOTVENDOR_NUM,
						 I_BOOTPRODUCT_NUM, I_BOOTVENDOR_STRING, I_BOOTPRODUCT_STRING,
						 I_BOOTSERIAL_STRING);
	if (d) {
		fprintf(stderr, "Found vusb adapter in bootloader mode\n");
		fprintf(stderr, "Booting it (This will take a few sec)\n");
		libusb_device_handle *handle;
		if (libusb_open(d, &handle)) {
			err("Failed to open device (bootloader)\n");
			return -EIO;
		}
		rfvusb_boot_adapter(handle);
		sleep(3);
		libusb_close(handle);
		return rfvusb_init(s, argc, argv);
	}
	a->dev = ncusb_find_dev(a->ctx, I_VENDOR_NUM, I_PRODUCT_NUM,
				I_VENDOR_STRING, I_PRODUCT_STRING, NULL);
	if (!a->dev) {
		err("No suitable device found ;( \n");
		return -ENODEV;
	}
	int err = libusb_open(a->dev, &a->h);
	if (err) {
		err("Failed to open device\n");
		return -EIO;
	}
	libusb_set_configuration(a->h, 1);
	libusb_claim_interface(a->h, 0);
	/* Allocate the data transfers. One for CB IO, one for ACB IO */

	a->cbtransfer = libusb_alloc_transfer(0);
	a->cbtransfer->user_data = a;

	a->acbtransfer = libusb_alloc_transfer(0);
	a->acbtransfer->user_data = a;

	a->cmdtransfer = libusb_alloc_transfer(0);
	a->cmdtransfer->user_data = a;

	ncusb_print_libusb_transfer(a->cbtransfer);
	ncusb_print_libusb_transfer(a->acbtransfer);

	/* Good, now let's fire the interrupt transfer */
	a->itransfer = libusb_alloc_transfer(0);
	libusb_fill_interrupt_transfer(a->itransfer, a->h, 0x81,
				       (unsigned char *) a->ibuf, 8, handle_interrupt_transfer, a, 30000);
	a->itransfer->user_data = a;
	libusb_submit_transfer(a->itransfer);
	return 0;
};

static int rfvusb_submit_transfer(struct rf24_transfer *t) {
	struct rf24_vusb_adaptor *a = (struct rf24_vusb_adaptor *) t->a;

	struct libusb_transfer *lt = a->cmdtransfer;
	switch(t->type) {
	case RF24_TRANSFER_SWEEP:
		dbg(
			"Starting sweep %d times timeout %d\n", t->sweep.sweeptimes, t->timeout_ms);
		libusb_fill_control_setup(a->xferbuffer,
					  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
					  | LIBUSB_ENDPOINT_IN, RQ_SWEEP, t->sweep.sweeptimes, 0,
					  128);
		break;
	case RF24_TRANSFER_CONFIG:
		a->failcounter = 0;    
		libusb_fill_control_setup(a->xferbuffer,
					  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
					  | LIBUSB_ENDPOINT_OUT, RQ_CONFIG, 0, 0,
					  sizeof(struct rf24_usb_config));
		memcpy(&a->xferbuffer[8], &t->conf, sizeof(struct rf24_usb_config));
		break;
	case RF24_TRANSFER_OPENPIPE:
		libusb_fill_control_setup(a->xferbuffer,
					  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
					  | LIBUSB_ENDPOINT_OUT, RQ_OPEN_PIPE, 0, 0, 6);
		a->xferbuffer[8] = t->pipeopen.pipe;
		memcpy(&a->xferbuffer[9], t->pipeopen.addr, 5);
		break;
	case RF24_TRANSFER_MODE:
		libusb_fill_control_setup(a->xferbuffer,
					  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE
					  | LIBUSB_ENDPOINT_OUT, RQ_MODE, t->mode, 0, 0);
		break;
	default:
		return 0; /* Do Nothing */
		break;
	}
    
	libusb_fill_control_transfer(lt, a->h, a->xferbuffer,
				     handle_simple_transfer, t, t->timeout_ms);

	return libusb_submit_transfer(lt);
}

static struct rf24_vusb_adaptor vusb_adaptor = {
	.a = {
		.name = "vusb-async",
		.usage = rfvusb_usage,
		.init = rfvusb_init,
		.submit_transfer = rfvusb_submit_transfer,
		.handle_events = rfvusb_handle_events,
	}
};


EXPORT_ADAPTOR(vusb_adaptor);
