#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>
#include "requests.h"
#include "adaptor.h"
#include "usb-helper.h"

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



#define CHECK(ret)							\
	if (ret<0) {							\
		fprintf(stderr, "%s: Transfer failed at %s:%d! Exiting!\n", \
			__FUNCTION__, __FILE__, __LINE__);		\
		exit(1);						\
	}								\
	
struct rf24_vusb_adaptor { 
	struct rf24_adaptor a;
	usb_dev_handle *h;
	int mode; 
};


static void do_control(usb_dev_handle *h, int rq, int value, int index)
{
	char tmp[256];
	int bytes = usb_control_msg(
		h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
		rq,      // bRequest
		value,              // wValue
		index,              // wIndex
		tmp,             // pointer to destination buffer
		256,  // wLength
		6000
		);
	CHECK(bytes);
}

static void fetch_debug(struct rf24_vusb_adaptor *a)
{
	if (!a->a.debug)
		return;
	char tmp[256];
	int bytes = usb_control_msg(
		a->h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
		RQ_NOP,      // bRequest
		0,              // wValue
		0,              // wIndex
		tmp,             // pointer to destination buffer
		256,  // wLength
		6000
		);
	CHECK(bytes);
	fprintf(stderr, "dbg: %d bytes: %s\n", bytes, tmp);
}

static int do_control_write(usb_dev_handle *h, int rq, char* buf, int len)
{
	char tmp[256];
	int bytes = usb_control_msg(
		h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_OUT, // bRequestType
		rq,      // bRequest
		0,              // wValue
		0,              // wIndex
		buf,             // pointer to source buffer
		len,  // wLength
		6000
		);
	CHECK(bytes);
	/* Fetch any debugging info */
	bytes = usb_control_msg(
		h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
		RQ_NOP,      // bRequest
		0,              // wValue
		0,              // wIndex
		tmp,             // pointer to destination buffer
		256,  // wLength
		6000
		);
	CHECK(bytes);

	if (strcmp(tmp,"O")==0) 
		return 0;
	if (strcmp(tmp,"F")==0) /* FULL */
		return -EAGAIN;
	if (strcmp(tmp,"E")==0) /* FULL */
		return -1;

	fprintf(stderr, "Unexpected response from dongle: %s \n"
		"Please rerun with --trace andfile a bug report\n"
		"The Application will now terminate\n\n", tmp
		);
	exit(1);
}

static void rf24_set_rconfig(void *s, int channel, int rate, int pa)
{
	struct rf24_vusb_adaptor *a = s;
	do_control(a->h, RQ_SET_RCONFIG, channel | (rate << 8), pa);
}

static void rf24_set_econfig(void *s, int nretries, int timeout, int streamcanfail)
{
	struct rf24_vusb_adaptor *a = s;
	if ((nretries <= 0) || (nretries > 15))
		nretries = 15;
	if ((timeout <= 0)  || (timeout  > 15))
		timeout = 15;
	do_control(a->h, RQ_SET_ECONFIG, nretries | (timeout << 8), streamcanfail);
}

static void rf24_set_local_addr(void *s, char *addr)
{
	struct rf24_vusb_adaptor *a = s;
	do_control_write(a->h, RQ_SET_LOCAL_ADDR, (char*) addr, 5);
}

static void rf24_set_remote_addr(void *s, char *addr)
{
	struct rf24_vusb_adaptor *a = s;
	do_control_write(a->h, RQ_SET_REMOTE_ADDR, (char*) addr, 5);
}

static int rf24_write(void *s, void *buffer, int size)
{
	struct rf24_vusb_adaptor *a = s;
	int rq = (a->mode == 1) ? RQ_STREAMWRITE : RQ_WRITE;
	return do_control_write(a->h, rq, buffer, size);
}

static int rf24_sweep(void *s, int times, uint8_t *buffer, int size)
{
	struct rf24_vusb_adaptor *a = s;
	int bytes = usb_control_msg(
		a->h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
		RQ_SWEEP,      // bRequest
		times,              // wValue
		0,              // wIndex
		(char*) buffer,             // pointer to destination buffer
		size,  // wLength
		6000
		);
	CHECK(bytes);
	return bytes;
}

static int rf24_read(void *s, void* buf, int size)
{
	struct rf24_vusb_adaptor *a = s;
	int rq = (a->mode == 1) ? RQ_STREAMREAD : RQ_READ;
	int bytes = usb_control_msg(
		a->h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
		rq,      // bRequest
		0,              // wValue
		0,              // wIndex
		buf,             // pointer to destination buffer
		size,  // wLength
		6000
		);
	CHECK(bytes);
	return bytes;
}

static void rf24_listen(void *s, int state, int stream)
{
	struct rf24_vusb_adaptor *a = s;
	a->mode = stream;
	do_control(a->h, RQ_LISTEN, state, stream);
	fetch_debug(a);
}


static char* usage = 
	"VUSB-based rf24tool adapter has no options so far \n";

static void rf24_usage() {
	fprintf(stderr, usage);
}

static int usb_set_report(usb_dev_handle *device, int reportType, char *buffer, int len)
{
	int bytesSent;
	
	bytesSent = usb_control_msg(device, USB_TYPE_CLASS | USB_RECIP_INTERFACE | USB_ENDPOINT_OUT, 
				    USBRQ_HID_SET_REPORT, reportType << 8 | 
				    buffer[0], 0, buffer, len, 5000);
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


static void rf24_boot_adapter(usb_dev_handle *h)
{
	deviceInfo_t inf;
	inf.reportId = 1; 	
        usb_set_report(h, USB_HID_REPORT_TYPE_FEATURE, (char*) &inf, sizeof(inf));
}


static int rf24_init(void *s, int argc, char* argv[]) {
	struct rf24_vusb_adaptor *a = s;
	usb_dev_handle *h = nc_usb_open(I_BOOTVENDOR_NUM, I_BOOTPRODUCT_NUM, 
					I_BOOTVENDOR_STRING, I_BOOTPRODUCT_STRING, I_BOOTSERIAL_STRING);
	if (h) { 
		fprintf(stderr, "Found vusb adapter in bootloader mode\n");
		fprintf(stderr, "Booting it (This will take a few sec)\n");
		rf24_boot_adapter(h);
		usb_close(h);
		sleep(3);
		return rf24_init(s, argc, argv);
	}

	a->h = nc_usb_open(I_VENDOR_NUM, I_PRODUCT_NUM, I_VENDOR_STRING, I_PRODUCT_STRING, NULL);
	if (!a->h) {
		fprintf(stderr, "No USB device found ;(\n");
		return 1;
	}
	usb_set_configuration(a->h, 1);
	usb_claim_interface(a->h, 0);
	return 0;
};

static int rf24_wsync(void *s)
{
	int qlen;
	//int qmax, fails;
	struct rf24_vusb_adaptor *a = s;
	char buffer[3];
	do { 
		int bytes = usb_control_msg(
			a->h,             // handle obtained with usb_open()
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
			RQ_POLL,      // bRequest
			0,              // wValue
			0,              // wIndex
			buffer,             // pointer to destination buffer
			3,  // wLength
			30000
			);
		CHECK(bytes);
		qlen = buffer[0];
//		qmax = buffer[1];
//		fails = buffer[2];
//		printf("\nqlen %d qmax %d fails %d\n", qlen, qmax, fails);
	} while (qlen); 
	return qlen;
}

struct rf24_vusb_adaptor uisp_adaptor = {
	.a = { 
		.name = "vusb",
		.usage = rf24_usage,
		.rf24_set_rconfig = rf24_set_rconfig,
		.rf24_set_econfig = rf24_set_econfig,
		.rf24_set_local_addr = rf24_set_local_addr,
		.rf24_set_remote_addr = rf24_set_remote_addr,
		.rf24_write = rf24_write,
		.rf24_sweep = rf24_sweep,
		.rf24_read = rf24_read,
		.rf24_listen = rf24_listen,
		.init = rf24_init,
		.rf24_wsync = rf24_wsync,
		
	}
};

EXPORT_ADAPTOR(uisp_adaptor);
