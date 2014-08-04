#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <libusb.h>
#include <stdint.h>
#include "requests.h"
#include "adaptor.h"

#define COMPONENT "vusb"
#include "dbg.h"

#define I_VENDOR_NUM        0x1d50
#define I_VENDOR_STRING     "www.ncrmnt.org"
#define I_PRODUCT_NUM       0x6032
#define I_PRODUCT_STRING    "nRF24L01-tool"

/* If we're in bootloader mode */ 
#define I_BOOTVENDOR_NUM        0x16c0
#define I_BOOTVENDOR_STRING     "www.ncrmnt.org"
#define I_BOOTPRODUCT_NUM       0x05df
#define I_BOOTPRODUCT_STRING    "Necromant's uISP"
#define I_BOOTSERIAL_STRING     "USB-NRF24L01-boot"

#define USB_HID_REPORT_TYPE_INPUT   1
#define USB_HID_REPORT_TYPE_OUTPUT  2
#define USB_HID_REPORT_TYPE_FEATURE 3
#define USBRQ_HID_SET_REPORT    0x09


struct rf24_vusb_adaptor { 
	struct rf24_adaptor          a;
	libusb_context              *ctx;
	struct libusb_device        *dev;
	struct libusb_device_handle *h;
	char                         ibuf[8];
	struct libusb_transfer      *itransfer;
	int mode; 
};

struct rfvusb_transferdata { 
	int pos; /* position in buffer */
	struct libusb_transfer *tf[2]; /* 2 buffers, ping-pong pattern */ 
};

/* convenience function to open the device */
static void ncusb_print_libusb_transfer(struct libusb_transfer *p_t)
{
	if ( NULL == p_t){
		info("Not a libusb_transfer...\n");
	}
	else {
		info("libusb_transfer structure:\n");
		info("flags =%x \n", p_t->flags);
		info("endpoint=%x \n", p_t->endpoint);
		info("type =%x \n", p_t->type);
		info("timeout =%d \n", p_t->timeout);
// length, and buffer are commands sent to the device
		info("length =%d \n", p_t->length);
		info("actual_length =%d \n", p_t->actual_length);
		info("buffer =%p \n", p_t->buffer);

	}
	return;
}


int ncusb_match_string(libusb_device_handle *dev, int index, const char* string)
{
	unsigned char tmp[256];
	libusb_get_string_descriptor_ascii(dev, index, tmp, 256);
	dbg("cmp idx %d str %s vs %s\n", index, tmp, string);
	if (string == NULL)
		return 1; /* NULL matches anything */
	return (strcmp(string, (char*) tmp)==0);
}


struct libusb_device *ncusb_find_dev(struct libusb_context *ctx, 
				     int vendor, int product, 
				     const char *vendor_name, 
				     const char *product_name, 
				     const char *serial)
{	libusb_device **list;
	libusb_device *found = NULL;
	
	ssize_t cnt = libusb_get_device_list(ctx, &list);
	ssize_t i = 0;
	int err = 0;

	if (cnt < 0){
		err( "no usb devices found\n" );
		return NULL;
	}

	for(i = 0; i < cnt; i++) {
		libusb_device *device = list[i];
		struct libusb_device_descriptor desc;
		libusb_device_handle *handle;	
		err = libusb_open(device, &handle);
		if (err) 
			continue;
		
		int r = libusb_get_device_descriptor( device, &desc );	
		if (r)
			continue;

		if ( desc.idVendor == vendor && desc.idProduct == product &&
		     ncusb_match_string(handle, desc.iManufacturer, vendor_name) &&
		     ncusb_match_string(handle, desc.iProduct,      product_name) &&
		     ncusb_match_string(handle, desc.iSerialNumber, serial)
			) 
		{ 
			found = device;
		}
		libusb_close(handle);

		if (found) 
			break;
		
	}
	
	return found;
}




static char* usage = 
	"VUSB-based rf24tool adapter has no options so far \n";

static void rfvusb_usage() {
	fprintf(stderr, usage);
}

static int usb_set_report(struct libusb_device_handle *h, int reportType, char *buffer, int len)
{
	int bytesSent;
	
	bytesSent = libusb_control_transfer(h, 
					    LIBUSB_REQUEST_TYPE_CLASS | 
					    LIBUSB_RECIPIENT_DEVICE | 
					    LIBUSB_ENDPOINT_OUT, 
					    USBRQ_HID_SET_REPORT, 
					    (reportType << 8 | buffer[0]), 
					    0, (unsigned char*) buffer, len, 5000);
	if(bytesSent != len) {
		if(bytesSent < 0)
			/* fprintf(stderr, "Error sending message: %s\n", usb_strerror()); */
		return -EIO;
	}
	return 0;
}

typedef struct deviceInfo {
	char    reportId;
	char    pageSize[2];
	char    flashSize[4];
} __attribute__((packed)) deviceInfo_t  ;


static void rfvusb_boot_adapter(struct libusb_device_handle *h)
{
	deviceInfo_t inf;
	inf.reportId = 1; 	
        usb_set_report(h, USB_HID_REPORT_TYPE_FEATURE, (char*) &inf, sizeof(inf));
}

static int rfvusb_handle_events_completed(void *s, int *completion)
{
	struct rf24_vusb_adaptor *a = s;
	return libusb_handle_events_completed (a->ctx, completion);
}

static int  rfvusb_handle_events(void *s)
{
	struct rf24_vusb_adaptor *a = s;
	return libusb_handle_events (a->ctx);
}

static void handle_interrupt_transfer(struct libusb_transfer *t)
{
	struct rf24_adaptor *a = t->user_data;

	if (t->status == LIBUSB_TRANSFER_COMPLETED ) { 
		a->status.buf_size      = (unsigned char)    t->buffer[1];
		a->status.buf_count     = (unsigned char)    t->buffer[0];
		a->status.ack_size      = (unsigned char)    t->buffer[3];
		a->status.ack_count     = (unsigned char)    t->buffer[2];
		a->status.num_failed    = (unsigned char)    t->buffer[4];
		a->status.clear_to_send = (unsigned char)    t->buffer[5];
		dbg("status: cb %u/%u acb %u/%u fails: %u qempty: %u\n",
		    t->buffer[0], t->buffer[1], t->buffer[2], t->buffer[3], 
		    t->buffer[4], t->buffer[5] );
	}
	
	libusb_submit_transfer(t);
}

static void handle_io_transfer(struct libusb_transfer *t)
{
	struct rf24_adaptor *a = t->user_data;
	if (t->status == LIBUSB_TRANSFER_COMPLETED ) { 
		/* Next one ?*/
	}
}


struct { int a; int b; } lookup[] = { 
	{ LIBUSB_TRANSFER_COMPLETED,  RF24_TRANSFER_COMPLETED },
	{ LIBUSB_TRANSFER_ERROR,      RF24_TRANSFER_ERROR     },
	{ LIBUSB_TRANSFER_TIMED_OUT,  RF24_TRANSFER_TIMED_OUT },
	{ LIBUSB_TRANSFER_CANCELLED,  RF24_TRANSFER_CANCELLED },
	{ LIBUSB_TRANSFER_STALL,      RF24_TRANSFER_ERROR     },
	{ LIBUSB_TRANSFER_NO_DEVICE,  RF24_TRANSFER_ERROR     },
	{ LIBUSB_TRANSFER_OVERFLOW,   RF24_TRANSFER_ERROR     },	
};

static int get_transfer_status(enum libusb_transfer_status s)
{
	int i;
	for (i=0; i<ARRAY_SIZE(lookup); i++)
		if (lookup[i].a == s)
			return lookup[i].b;
}
static void handle_simple_transfer(struct libusb_transfer *lt)
{
	struct rf24_transfer *t = lt->user_data;
	t->status = get_transfer_status(lt->status);
	if (t->type == RF24_TRANSFER_SWEEP) { 
		memmove(lt->buffer, &lt->buffer[8], 128);
	} 
	rf24_callback(t);
}


/** 
 * Allocate per-transfer private data. 
 * 
 * @param t transfer
 * 
 * @return 0 - success, else failure code
 */
int rfvusb_alloc_transferdata(struct rf24_transfer *t) 
{
	t->adaptordata = calloc(sizeof(struct rfvusb_transferdata), 1);
	return (t->adaptordata == NULL) ? -ENOMEM : 0;
}

static int rfvusb_init(void *s, int argc, char* argv[]) {
	struct rf24_vusb_adaptor *a = s;
	libusb_init(&a->ctx);
	struct libusb_device *d = ncusb_find_dev(a->ctx, I_BOOTVENDOR_NUM, I_BOOTPRODUCT_NUM, 
					I_BOOTVENDOR_STRING, I_BOOTPRODUCT_STRING, I_BOOTSERIAL_STRING);
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
	a->dev = ncusb_find_dev(a->ctx, I_VENDOR_NUM, I_PRODUCT_NUM, I_VENDOR_STRING, I_PRODUCT_STRING, NULL);
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
	/* Good, now let's fire the interrupt transfer */
	a->itransfer = libusb_alloc_transfer(0);
	libusb_fill_interrupt_transfer (a->itransfer, 
					a->h, 
					0x81, (unsigned char *) a->ibuf, 8, handle_interrupt_transfer, a, 30000);
	a->itransfer->user_data = a;
	libusb_submit_transfer(a->itransfer);
	return 0;
};

static int rfvusb_submit_transfer(struct rf24_transfer *t)
{
	struct rf24_vusb_adaptor *a = (struct rf24_vusb_adaptor *)t->a;
	struct libusb_transfer *lt = libusb_alloc_transfer(0); 
	if (!lt)
		return -ENOMEM;
	
	if (t->type == RF24_TRANSFER_SWEEP)
	{
		dbg("Starting sweep %d times timeout %d\n", t->sweep.sweeptimes, t->timeout_ms);
		libusb_fill_control_setup((unsigned char *)t->sweep.sweepdata, 
					  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | 
					  LIBUSB_ENDPOINT_IN,
					  RQ_SWEEP, t->sweep.sweeptimes, 0, 128);
		libusb_fill_control_transfer(lt, a->h, 
					     (unsigned char *)t->sweep.sweepdata, handle_simple_transfer, 
					     t, t->timeout_ms);
	} else if (t->type == RF24_TRANSFER_CONFIG) { 
		unsigned char* tmp = malloc(sizeof(struct rf24_config) + 8);
		if (!tmp) 
			goto errfreetransfer; 
		lt->flags = LIBUSB_TRANSFER_FREE_BUFFER | LIBUSB_TRANSFER_FREE_TRANSFER; 
		libusb_fill_control_setup(tmp, 
					  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | 
					  LIBUSB_ENDPOINT_OUT,
					  RQ_CONFIG, 0, 0, sizeof(struct rf24_config));
		memcpy(&tmp[8], &t->conf, sizeof(struct rf24_config));
		libusb_fill_control_transfer(lt, a->h, 
					     tmp, handle_simple_transfer, 
					     t, t->timeout_ms);
		
	} else if (t->type == RF24_TRANSFER_OPENPIPE) {
		unsigned char* tmp = malloc(6 + 8);
		if (!tmp) 
			goto errfreetransfer; 
		lt->flags = LIBUSB_TRANSFER_FREE_BUFFER | LIBUSB_TRANSFER_FREE_TRANSFER; 
		libusb_fill_control_setup(tmp, 
					  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | 
					  LIBUSB_ENDPOINT_OUT,
					  RQ_OPEN_PIPE, 0, 0, 6);
		tmp[8] = t->pipeopen.pipe;
		memcpy(&tmp[9], t->pipeopen.addr, 5);
		libusb_fill_control_transfer(lt, a->h, 
					     tmp, handle_simple_transfer, 
					     t, t->timeout_ms);
	} else {
		/* For read and writes we allocate 2 transfers 
		   that will operate in ping-pong fasion */  
		struct rfvusb_transferdata *pdata = t->adaptordata; 
		pdata->tf[0] = lt;
		pdata->tf[1] = libusb_alloc_transfer(0); 
		int rq; 
		switch(t->type) { 

		case RF24_TRANSFER_READ:
			
			break;

		case RF24_TRANSFER_WRITE:
			rq = RQ_WRITE; 
			break;
		default:
			err("This shouldn't happen. Internal librf24 BUG!\n");
			exit(1);
		};
		
//		libusb_fill_control_setup(tmp, 
//					  LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | 
//					  LIBUSB_ENDPOINT_OUT,
//					  rq, 0, 0, 32);
		
		
	}
	return libusb_submit_transfer(lt);
errfreetransfer:
	free(lt);
	return -ENOMEM;
	
}


struct rf24_vusb_adaptor vusb_adaptor = {
	.a = { 
		.name                     = "vusb-async",
		.usage                    = rfvusb_usage,
		.init                     = rfvusb_init,

//		.open_reading_pipe        = rfvusb_open_reading_pipe,
//		.open_writing_pipe        = rfvusb_open_writing_pipe,

//		.write                    = rfvusb_write,
//		.read                     = rfvusb_read,
//		.set_ack_payload          = rfvusb_set_ack_payload,
//		.mode                     = rfvusb_mode,
//		.sync                     = rfvusb_sync,
//		.flush                    = rfvusb_flush,
	
		.submit_transfer          = rfvusb_submit_transfer, 
		.handle_events_completed  = rfvusb_handle_events_completed,
		.handle_events            = rfvusb_handle_events,
		.allocate_transferdata    = rfvusb_alloc_transferdata

	}
};

EXPORT_ADAPTOR(vusb_adaptor);
