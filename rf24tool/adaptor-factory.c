#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>
#include "requests.h"
#include "adaptor.h"


static struct rf24_adaptor *adaptors; 
void rf24tool_register_adaptor(void *p)
{
	struct rf24_adaptor *a = p;
	if (!adaptors)
		adaptors = a; 
	
	struct rf24_adaptor *ad = adaptors; 
	while (ad->next) 
		ad = ad->next;
	ad->next = a;
	a->next = NULL;
}

struct rf24_adaptor *rf24_get_adaptor_by_name(char* name)
{
	struct rf24_adaptor *a = adaptors; 
	
	while (a) { 
		if (strcmp(a->name, name)==0)
			return a;
		a = a->next;
	}
	return NULL;
}

struct rf24_adaptor *rf24_get_default_adaptor()
{
	return adaptors;
}

