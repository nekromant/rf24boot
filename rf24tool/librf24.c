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

int  rf24_handle_events(struct rf24_adaptor *a)
{
	return a->handle_events(a);
}


struct rf24_transfer *rf24_allocate_io_transfer(struct rf24_adaptor *a, int npackets)
{
	struct rf24_transfer *t = calloc(sizeof(struct rf24_transfer), 1); 
	if (!t)
		return NULL;
	t->a = a;
	
	t->io.data = calloc(npackets, sizeof(struct rf24_packet));
	if (!t->io.data) 
		goto errfreetransfer; 
	
	t->io.ackdata = calloc(npackets, sizeof(struct rf24_packet));
	if (!t->io.ackdata) 
		goto errfreedata; 
	
	t->io.num_allocated = npackets; 

	t->timeout_ms = 1000; /* 1000 ms, a reasonable default */

	if ((!a->allocate_transferdata) || (0==a->allocate_transferdata(t)))
		return t;

errfreeackdata: 
	free(t->io.ackdata);

errfreedata: 
	free(t->io.data);
	
errfreetransfer:
	free(t);
	return NULL;
}

struct rf24_transfer *rf24_allocate_sweep_transfer(struct rf24_adaptor *a, int times)
{
	struct rf24_transfer *t = calloc(sizeof(struct rf24_transfer), 1); 
	if (!t)
		return NULL;
	t->a = a;	
	t->type = RF24_TRANSFER_SWEEP;
	t->sweep.sweeptimes = times;
	t->timeout_ms = 400*times; /* Give it ~1ms per channel */
	return t;
}

struct rf24_transfer *rf24_allocate_config_transfer(struct rf24_adaptor *a, struct rf24_config *conf)
{
	struct rf24_transfer *t = calloc(sizeof(struct rf24_transfer), 1); 
	if (!t)
		return NULL;
	t->a = a;	
	t->type = RF24_TRANSFER_CONFIG;
	t->conf = *conf;
	t->timeout_ms = 1000;
	return t;
	
}

struct rf24_transfer *rf24_allocate_openpipe_transfer(struct rf24_adaptor *a, enum rf24_pipe pipe, char* addr)
{
	struct rf24_transfer *t = calloc(sizeof(struct rf24_transfer), 1); 
	if (!t)
		return NULL;
	t->a = a;	
	t->type = RF24_TRANSFER_OPENPIPE;
	t->pipeopen.pipe = pipe; 
	memcpy(t->pipeopen.addr, addr, 5); /* copy the address */
	t->timeout_ms = 1000;
	return t;
}

int rf24_set_transfer_timeout(struct rf24_transfer *t, int timeout_ms)
{
	t->timeout_ms = timeout_ms; 
}

int rf24_set_transfer_callback(struct rf24_transfer *t, rf24_transfer_cb_fn callback)
{
	t->callback = callback; 
}

int rf24_submit_transfer(struct rf24_transfer *t) 
{
	if ((t->status == RF24_TRANSFER_RUNNING) || (t->status == RF24_TRANSFER_CANCELLING)) {
		warn("Attempt to submit a running/cancelling transfer\n");
		return -EAGAIN; /* Already running? */
	}
	
	t->timeout_ms_left  = t->timeout_ms;
	t->status           = RF24_TRANSFER_RUNNING;
	
	/* TODO: More sanity */
	struct rf24_adaptor *a = t->a;
	return a->submit_transfer(t);
}

void rf24_callback(struct rf24_transfer *t)
{
	t->callback(t);
	if (t->flags & RF24_TRANSFER_FLAG_FREE) {
		free(t);
	}
}

void rf24_cancel_transfer(struct rf24_transfer *t)
{
	/* TODO: sanity */
}
