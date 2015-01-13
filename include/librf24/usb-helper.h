#ifndef USBHELPER_H
#define USBHELPER_H

usb_dev_handle *nc_usb_open(int vendor, int product, char *vendor_name, char *product_name, char *serial);

usb_dev_handle *usb_check_device(struct usb_device *dev,
                                 char *vendor_name, 
                                 char *product_name, 
                                 char *serial);

int usb_match_string(usb_dev_handle *handle, int index, char* string);




#endif
