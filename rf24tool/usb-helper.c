
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>

static int did_usb_init = 0;


static int  usb_get_string_ascii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen)
{
	char    buffer[256];
	int     rval, i;
	
	if((rval = usb_control_msg(dev, 
				   USB_ENDPOINT_IN, 
				   USB_REQ_GET_DESCRIPTOR, 
				   (USB_DT_STRING << 8) + index, 
				   langid, buffer, sizeof(buffer), 
				   1000)) < 0)
		return rval;
	if(buffer[1] != USB_DT_STRING)
		return 0;
	if((unsigned char)buffer[0] < rval)
		rval = (unsigned char)buffer[0];
	rval /= 2;
	/* lossy conversion to ISO Latin1 */
	for(i=1; i<rval; i++) {
		if(i > buflen)  /* destination buffer overflow */
			break;
		buf[i-1] = buffer[2 * i];
		if(buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
			buf[i-1] = '?';
	}
	buf[i-1] = 0;
	return i-1;
}


int usb_match_string(usb_dev_handle *handle, int index, char* string)
{
	char tmp[256];
	if (string == NULL)
		return 1; /* NULL matches anything */
	usb_get_string_ascii(handle, index, 0x409, tmp, 256);
	return (strcmp(string,tmp)==0);
}

usb_dev_handle *usb_check_device(struct usb_device *dev,
				 char *vendor_name, 
				 char *product_name, 
				 char *serial)
{
	usb_dev_handle      *handle = usb_open(dev);
	if(!handle) {
		fprintf(stderr, "Warning: cannot open USB device: %s\n", usb_strerror());
		return NULL;
	}
	if (
		usb_match_string(handle, dev->descriptor.iManufacturer, vendor_name) &&
		usb_match_string(handle, dev->descriptor.iProduct,      product_name) &&
		usb_match_string(handle, dev->descriptor.iSerialNumber, serial)
		) {
		return handle;
	}
	usb_close(handle);
	return NULL;
	
}

usb_dev_handle *nc_usb_open(int vendor, int product, char *vendor_name, char *product_name, char *serial)
{
	struct usb_bus      *bus;
	struct usb_device   *dev;
	usb_dev_handle      *handle = NULL;
	
	if(!did_usb_init++)
		usb_init();

	usb_find_busses();
	usb_find_devices();

	for(bus=usb_get_busses(); bus; bus=bus->next) {
		for(dev=bus->devices; dev; dev=dev->next) {
			            if(dev->descriptor.idVendor == vendor && 
				       dev->descriptor.idProduct == product) {
					    handle = usb_check_device(dev, vendor_name, product_name, serial);
					    if (handle)
						    return handle;
				    }
		}
	}
	return NULL;
}

