#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>

#include "requests.h"
#include "rf24.h"


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
//	fprintf(stderr, "dbg %d bytes: %s\n", bytes, tmp);
}

static void fetch_debug(usb_dev_handle *h)
{
	char tmp[256];
	int bytes = usb_control_msg(
		h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
		RQ_NOP,      // bRequest
		0,              // wValue
		0,              // wIndex
		tmp,             // pointer to destination buffer
		256,  // wLength
		6000
		);
//	fprintf(stderr, "dbg %d bytes: %s\n", bytes, tmp);
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
	//fprintf(stderr, "send: %s\n", tmp);
	if (strcmp(tmp,"OK")==0) 
		return 0;
	return 1;
}

void rf24_set_config(usb_dev_handle *h, int channel, int rate, int pa)
{
	do_control(h, RQ_SET_CONFIG, channel | (rate << 8), pa);
}

void rf24_set_local_addr(usb_dev_handle *h, uint8_t *addr)
{
	do_control_write(h, RQ_SET_LOCAL_ADDR, addr, 5);
}

void rf24_set_remote_addr(usb_dev_handle *h, uint8_t *addr)
{
	do_control_write(h, RQ_SET_REMOTE_ADDR, addr, 5);
}

int rf24_write(usb_dev_handle *h, uint8_t *buffer, int size)
{
	do_control_write(h, RQ_WRITE, buffer, size);
}

int rf24_read(usb_dev_handle *h, uint8_t *buffer, int size)
{
	int bytes = usb_control_msg(
		h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
		RQ_READ,      // bRequest
		0,              // wValue
		0,              // wIndex
		buffer,             // pointer to destination buffer
		size,  // wLength
		6000
		);
	return bytes;
}

void rf24_open_pipes(usb_dev_handle *h)
{
	do_control(h, RQ_OPEN_PIPES, 0, 0);
	fetch_debug(h);
}

void rf24_listen(usb_dev_handle *h, int state)
{
	do_control(h, RQ_LISTEN, state, 0);
	fetch_debug(h);
}

