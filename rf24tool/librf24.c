#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>
#include "requests.h"
#include "adaptor.h"


/* This wrapper calls the underlying rf24 code and performs additional sanity checks */

/** 
 * Set the adaptor configuration
 * 
 * @param a adaptor
 * @param conf configuration struct to apply
 */
void rf24_config(struct rf24_adaptor *a, struct rf24_config *conf)
{
	/* cache config locally */
	if (!a->conf)
		a->conf = malloc(sizeof(*conf));
	memcpy(a->conf, conf, sizeof(*conf)); /* Store config locally */
	/* TODO: Sanity checking */
	a->config(a, conf);
}


void rf24_mode(struct rf24_adaptor *a, int mode)
{
	/* TODO: mode sanity checking */
	a->mode(a, mode);
}

void rf24_open_reading_pipe(struct rf24_adaptor *a, int pipe, char *addr)
{
	a->open_reading_pipe(a, pipe, addr);
}
	
void rf24_open_writing_pipe(struct rf24_adaptor *a, char *addr)
{
	a->open_writing_pipe(a, addr);
}

int rf24_write(struct rf24_adaptor *a, void *buffer, int size) 
{
	return a->write(a, buffer, size); 
}

int rf24_read(struct rf24_adaptor *a, int *pipe, void *buffer, int size)
{
	return a->read(a, pipe, buffer, size); 
}

/** 
 * Synchronize. Block until dongle sends out all buffered data. 
 * Do not call this when reading 
 *
 * @param a adaptor
 * @param timeout timeout in 10ms fractions
 * 
 * @return number of timeout fractions remaining or 0 if synchronisationfailed
 */
int rf24_sync(struct rf24_adaptor *a, uint16_t timeout)
{
	return a->sync(a, timeout);
}

void rf24_flush(struct rf24_adaptor *a)
{
	a->flush(a);
}

int rf24_init(struct rf24_adaptor *a, int argc, char *argv[])
{
	return a->init(a, argc, argv);
}

int rf24_sweep(struct rf24_adaptor *a, int times, uint8_t *buffer, int size)
{
	return a->sweep(a, times, buffer, size);
}
