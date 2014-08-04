#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>
#include "requests.h"
#include "adaptor.h"

#define COMPONENT "librf24"
#include "dbg.h"


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

void rf24_set_transfer_timeout(struct rf24_transfer *t, int timeout_ms)
{
	t->timeout_ms = timeout_ms; 
}

void rf24_set_transfer_callback(struct rf24_transfer *t, rf24_transfer_cb_fn callback)
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
	dbg("Submitting transfer\n");
	return a->submit_transfer(t);
}


void rf24_free_transfer(struct rf24_transfer *t)
{
	if ((t->type == RF24_TRANSFER_READ) || (t->type == RF24_TRANSFER_WRITE)) {
		free(t->io.data);
		free(t->io.ackdata);
	}
	free(t);
}
void rf24_callback(struct rf24_transfer *t)
{
	if (t->callback)
		t->callback(t);
	if (t->flags & RF24_TRANSFER_FLAG_FREE)
		rf24_free_transfer(t);
}

void rf24_cancel_transfer(struct rf24_transfer *t)
{
	/* TODO: sanity */
}

int rf24_make_write_transfer(struct rf24_transfer *t, int mode) 
{
	if (mode <= MODE_READ) { 
		warn("Invalid mode supplied to rf24_make_write_transfer\n");
		return 1;
	}

	t->type                = RF24_TRANSFER_WRITE; 
	t->io.transfermode     = mode;
	t->io.num_data_packets = 0;
	t->io.num_ack_packets  = 0;
	/* TODO: Sanity checking */
	return 0;
}

/** 
 * Add a packet to a transfer. Call this function after calling 
 * rf24_make_read_transfer() or rf24_make_write_transfer()
 * When a transfer is WRITE this adds the packets to the outgoing list of packets, 
 * pipe argument is ignored. 
 * When the transfer is READ, with ack payloads enabled it adds the packet to the 
 * ack packets for the specified pipe.  
 * There's no reason to call this function when ack payloads are disabled.  
 * 
 * @param t transfer, must be either a READ or WRITE
 * @param data Data to put into the packet, up to 32 bytes
 * @param len length 1-32
 * @param pipe number of pipe (for ack packets)
 * 
 * @return 0 - ok, else failure code
 */
int rf24_add_packet(struct rf24_transfer *t, char* data, int len, int pipe)
{
	struct rf24_packet *p;
	if (len>32)
		return -EBADF; /* too much data */
	if (t->io.transfermode == RF24_TRANSFER_WRITE) {
		if (t->io.num_data_packets >= t->io.num_allocated) 
			return -ENOMEM;
		p = &t->io.data[t->io.num_data_packets++];
	} else if (t->io.transfermode == RF24_TRANSFER_READ) {
		if (t->io.num_data_packets >= t->io.num_allocated) 
			return -ENOMEM;
		p = &t->io.ackdata[t->io.num_ack_packets++];
	} else 
		return -EIO; /* Wrong type of transfer */
	
	p->pipe = pipe;
        memcpy(p->data, data, len);
	return 0;
}
