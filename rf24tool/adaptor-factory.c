#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <dlfcn.h>
#include <stdint.h>
#include <usb.h>
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

void rf24_list_adaptors()
{
	struct rf24_adaptor *a = adaptors; 
	fprintf(stderr, "Available adaptors: \n");
	while (a) { 
		fprintf(stderr, "*  %s\n",a->name);
		a = a->next;
	}
}

struct rf24_adaptor *rf24_get_default_adaptor()
{
	return adaptors;
}


static void __attribute__ ((constructor)) load_dynamic_adaptors() 
{
	dlopen("libantares.so", RTLD_NOW);
}
