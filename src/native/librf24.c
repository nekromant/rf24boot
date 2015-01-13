#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>
#include "requests.h"
#include <librf24/adaptor.h>

#define COMPONENT "librf24"
#include <lib/printk.h>


struct rf24_transfer *librf24_allocate_io_transfer(struct rf24_adaptor *a, int npackets)
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

struct rf24_transfer *librf24_allocate_sweep_transfer(struct rf24_adaptor *a, int times)
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

struct rf24_transfer *librf24_allocate_config_transfer(struct rf24_adaptor *a, struct rf24_config *conf)
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

struct rf24_transfer *librf24_allocate_openpipe_transfer(struct rf24_adaptor *a, enum rf24_pipe pipe, char* addr)
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

void librf24_set_transfer_timeout(struct rf24_transfer *t, int timeout_ms)
{
	t->timeout_ms = timeout_ms; 
}

void librf24_set_transfer_callback(struct rf24_transfer *t, rf24_transfer_cb_fn callback)
{
	t->callback = callback; 
}

int librf24_submit_transfer(struct rf24_transfer *t) 
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


void librf24_free_transfer(struct rf24_transfer *t)
{
	if ((t->type == RF24_TRANSFER_READ) || (t->type == RF24_TRANSFER_WRITE)) {
		free(t->io.data);
		free(t->io.ackdata);
	}
	free(t);
}
void librf24_callback(struct rf24_transfer *t)
{
	if (t->callback)
		t->callback(t);
	if (t->flags & RF24_TRANSFER_FLAG_FREE)
		rf24_free_transfer(t);
}

void librf24_cancel_transfer(struct rf24_transfer *t)
{
	/* TODO: sanity */
}

int librf24_make_write_transfer(struct rf24_transfer *t, int mode) 
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
int librf24_add_packet(struct rf24_transfer *t, char* data, int len, int pipe)
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
