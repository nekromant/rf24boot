#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>

#include "requests.h"
#include "rf24.h"
#include <errno.h>


static int trace = 0; 


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
	return bytes;
}

void rf24_set_config(usb_dev_handle *h, int channel, int rate, int pa)
{
	TRACE();
	do_control(h, RQ_SET_CONFIG, channel | (rate << 8), pa);
}

void rf24_set_local_addr(usb_dev_handle *h, uint8_t *addr)
{
	TRACE();
	do_control_write(h, RQ_SET_LOCAL_ADDR, addr, 5);
}

void rf24_set_remote_addr(usb_dev_handle *h, uint8_t *addr)
{
	TRACE();
	do_control_write(h, RQ_SET_REMOTE_ADDR, addr, 5);
}


int rf24_sweep(usb_dev_handle *h, int times, uint8_t *buffer, int size)
{
	TRACE();
	int bytes = usb_control_msg(
		h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
		RQ_SWEEP,      // bRequest
		times,              // wValue
		0,              // wIndex
		buffer,             // pointer to destination buffer
		size,  // wLength
		6000
		);
	return bytes;
}

void rf24_open_pipes(usb_dev_handle *h)
{
	TRACE();
	do_control(h, RQ_OPEN_PIPES, 0, 0);
	fetch_debug(h);
}

void rf24_listen(usb_dev_handle *h, int state)
{
	TRACE();
	do_control(h, RQ_LISTEN, state, 0);
	fetch_debug(h);
}

void rf24_get_status(usb_dev_handle *h, int *qlen, int *qmax, int *fails)
{
	unsigned char buffer[3];
	int bytes = usb_control_msg(
		h,             // handle obtained with usb_open()
		USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
		RQ_POLL,      // bRequest
		0,              // wValue
		0,              // wIndex
		buffer,             // pointer to destination buffer
		3,  // wLength
		30000
		);
	*qlen = buffer[0];
	*qmax = buffer[1];
	*fails = buffer[2];
}


int rf24_read(usb_dev_handle *h, uint8_t *buffer, int size)
{
	TRACE();
	int ql,qm,f;
	rf24_get_status(h, &ql, &qm, &f); 
	if (ql) { 
		int bytes = usb_control_msg(
			h,             // handle obtained with usb_open()
			USB_TYPE_VENDOR | USB_RECIP_DEVICE | USB_ENDPOINT_IN, // bRequestType
			RQ_READ,      // bRequest
			0,              // wValue
			0,              // wIndex
			buffer,             // pointer to destination buffer
			size,  // wLength
			30000
			);
		return bytes;
	};
	return 0;
}

int rf24_write(usb_dev_handle *h, uint8_t *buffer, int size)
{
	TRACE();
	int ql,qm,f;
	rf24_get_status(h, &ql, &qm, &f); 
	if (ql < qm) { 
		return do_control_write(h, RQ_WRITE, buffer, size);
	}
	return -EAGAIN;
}

int rf24_wsync(usb_dev_handle *h, int timeout)
{
	TRACE();
	int ql,qm,f;
	do { 
		rf24_get_status(h, &ql, &qm, &f); 
	} while (ql);
	return f; /* Return number of failed pkts */
}
