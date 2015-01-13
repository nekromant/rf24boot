#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <usb.h>
#include <libusb.h>
#include <stdint.h>
#include <time.h>
#include "librf24-private.h"

#define DEBUG_LEVEL 5
#define COMPONENT "librf24"
#include "dbg.h"


int rf24_init(struct rf24_adaptor *a, int argc, char *argv[]) 
{
	/* Prepare dummy modeswitch transfer */
	struct rf24_transfer *t = &a->modeswitch_transfer;
	memset(t, 0x0, sizeof(struct rf24_transfer));
	t->a = a;
	t->type = RF24_TRANSFER_MODE;
	t->timeout_ms = 1000; /* Give it ~1s */
	a->current_mode = RF24_MODE_INVALID;
	if ((a->allocate_transferdata) && (0 != a->allocate_transferdata(t)))
		return -EIO;

	return a->init(a, argc, argv);
}

int rf24_handle_events(struct rf24_adaptor *a) {
	return a->handle_events(a);
}

long get_timestamp()
{
	long ms; 
	struct timeval tv; 
	gettimeofday(&tv, NULL);
	ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	return ms;
}

static int transfer_is_timeouted(struct rf24_transfer *t)
{
	if (get_timestamp() > (t->time_started + t->timeout_ms)) { 
		t->status = RF24_TRANSFER_TIMED_OUT;
		rf24_callback(t);
		return 1;
	}
	return 0;
}

struct rf24_transfer *rf24_allocate_io_transfer(struct rf24_adaptor *a,
		int npackets) {
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

	if ((!a->allocate_transferdata) || (0 == a->allocate_transferdata(t)))
		return t;

	errfreedata: free(t->io.data);

	errfreetransfer: free(t);
	return NULL;
}

struct rf24_transfer *rf24_allocate_sweep_transfer(struct rf24_adaptor *a,
		int times) {
	struct rf24_transfer *t = calloc(sizeof(struct rf24_transfer), 1);
	if (!t)
		return NULL;
	t->a = a;
	t->type = RF24_TRANSFER_SWEEP;
	t->sweep.sweeptimes = times;
	t->timeout_ms = 400 * times; /* Give it ~1ms per channel */
	if ((!a->allocate_transferdata) || (0 == a->allocate_transferdata(t)))
		return t;
	free(t);
	return NULL;
}


struct rf24_transfer *rf24_allocate_modeswitch_transfer(struct rf24_adaptor *a,
		enum rf24_mode mode) {
	struct rf24_transfer *t = calloc(sizeof(struct rf24_transfer), 1);
	if (!t)
		return NULL;
	t->a = a;
	t->type = RF24_TRANSFER_MODE;
	t->mode = mode;
	t->timeout_ms = 1000; /* Give it ~1s */
	if ((!a->allocate_transferdata) || (0 == a->allocate_transferdata(t)))
		return t;
	free(t);
	return NULL;
}

struct rf24_transfer *rf24_allocate_config_transfer(struct rf24_adaptor *a,
		struct rf24_usb_config *conf) {
	struct rf24_transfer *t = calloc(sizeof(struct rf24_transfer), 1);
	if (!t)
		return NULL;
	t->a = a;
	t->type = RF24_TRANSFER_CONFIG;
	t->conf = *conf;
	t->timeout_ms = 1000;
	if ((!a->allocate_transferdata) || (0 == a->allocate_transferdata(t)))
		return t;
	free(t);
	return NULL;
}

struct rf24_transfer *rf24_allocate_openpipe_transfer(struct rf24_adaptor *a,
		enum rf24_pipe pipe, const char* addr) {
	struct rf24_transfer *t = calloc(sizeof(struct rf24_transfer), 1);
	if (!t)
		return NULL;
	t->a = a;
	t->type = RF24_TRANSFER_OPENPIPE;
	t->pipeopen.pipe = pipe;
	memcpy(t->pipeopen.addr, addr, 5); /* copy the address */
	t->timeout_ms = 1000;
	if ((!a->allocate_transferdata) || (0 == a->allocate_transferdata(t)))
		return t;
	free(t);
	return NULL;
}

void rf24_set_transfer_timeout(struct rf24_transfer *t, int timeout_ms) {
	t->timeout_ms = timeout_ms;
}

void rf24_set_transfer_callback(struct rf24_transfer *t,
		rf24_transfer_cb_fn callback) {
	t->callback = callback;
}


static int dispatch_next_transfer(struct rf24_transfer *t)
{
	struct rf24_adaptor *a = t->a;

	/* In case adaptor code's incomplete */
	if (!a->submit_transfer)
		return 1;

	/* Store current mode */
	if (t->type == RF24_TRANSFER_MODE) {
		a->current_mode = t->mode;
	}

	/* 
	 * If we're reading or writing, we may need to change mode first.
	 * This is done by submitting a dummy modeswitch transfer BEFORE
	 * firing the actual IO transfer 
	 */	

	if ((RF24_TRANSFER_IS_IO(t)) && 
	    (a->current_mode != t->io.transfermode)) { 
		dbg("We need to switch modes, sending modeswitcher transfer!\n");
		a->modeswitch_transfer.mode = t->io.transfermode;
		a->modeswitch_transfer.next = t;
		a->current_transfer = &a->modeswitch_transfer;
		return dispatch_next_transfer(&a->modeswitch_transfer);
		}

	return a->submit_transfer(t);
}

static void start_next_transfer(struct rf24_adaptor *a, struct rf24_transfer *next)
{
	if (next) {
		a->current_transfer = next;
		/* Adaptor refused to submit the next transfer ? */
		if (0 != dispatch_next_transfer(next)) {
			next->status = RF24_TRANSFER_ERROR;
			rf24_callback(next); /* Fire the callback, we failed. */
		}
		
	} else {
		a->current_transfer = NULL;
	};
}


void rf24_callback(struct rf24_transfer *t) {
	dbg("Callbacking!\n");
	/* Fire the callback! */
	if (t->callback)
		t->callback(t);

	/* TODO: cancellation point! any timed out transfers ? */

	struct rf24_adaptor *a = t->a;
	/* Do we have any more transfers queued? Fire the next one current */
	start_next_transfer(a, t->next);
	/* Should we free our transfer ? */
	if (t->flags & RF24_TRANSFER_FLAG_FREE)
		rf24_free_transfer(t);
}

int rf24_submit_transfer(struct rf24_transfer *t) {
	struct rf24_adaptor *a = t->a;

	if ((t->status == RF24_TRANSFER_RUNNING)
			|| (t->status == RF24_TRANSFER_CANCELLING)) {
		warn("Attempt to submit a running/cancelling transfer\n");
		return -EAGAIN; /* Already running? */
	}

	t->timeout_ms_left = t->timeout_ms;
	t->time_started    = get_timestamp();
	t->status = RF24_TRANSFER_RUNNING;

	if (RF24_TRANSFER_IS_IO(t)) {
		t->io.num_data_packets_xfered = 0;
		t->io.num_data_packets_failed = 0;
		t->io.num_ack_packets_xfered  = 0;
	}

	dbg("Submitting transfer, type %d \n", t->type);
	/* TODO: Set timestamp */

	t->next = NULL;

	if (a->current_transfer)  {
		dbg("queued!\n");
		a->last_transfer->next = t;
		a->last_transfer = t;
	} else {
		/* Add to the linked list */
		dbg("started!\n");
		a->current_transfer = t;
		a->last_transfer    = t;
		/* First transfer: submit! */
		if ( 0 != dispatch_next_transfer(t)) {
			t->status = RF24_TRANSFER_ERROR;
			rf24_callback(t); /* Fire the callback, we failed. */
		}
	}
	return 0;
}

void rf24_free_transfer(struct rf24_transfer *t) 
{
	if ((t->type == RF24_TRANSFER_READ) || (t->type == RF24_TRANSFER_WRITE)) {
		free(t->io.data);
		free(t->io.ackdata);
	}
	if (t->adaptordata)
		free(t->adaptordata);
	free(t);
}

int rf24_cancel_transfer(struct rf24_transfer *t) {
	if (t->status = RF24_TRANSFER_RUNNING) { 
		t->status = RF24_TRANSFER_CANCELLING;
		return 0;
	}
	return -EIO;
}


int rf24_make_read_transfer(struct rf24_transfer *t, int numpackets) {
	t->type = RF24_TRANSFER_READ;
	t->io.transfermode = MODE_READ;
	t->io.num_data_packets = numpackets;
	t->io.num_ack_packets = 0;
	/* TODO: Sanity checking */
	return 0;
}

int rf24_make_write_transfer(struct rf24_transfer *t, int mode) {
	if (mode <= MODE_READ) {
		warn("Invalid mode supplied to rf24_make_write_transfer\n");
		return 1;
	}

	t->type = RF24_TRANSFER_WRITE;
	t->io.transfermode = mode;
	t->io.num_data_packets = 0;
	t->io.num_ack_packets = 0;
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
int rf24_add_packet(struct rf24_transfer *t, char* data, int len, int pipe) {
	struct rf24_packet *p;
	if (len > 32)
		return -EBADF; /* too much data */
	if (t->type == RF24_TRANSFER_WRITE) {
		if (t->io.num_data_packets >= t->io.num_allocated)
			return -ENOMEM;
		p = &t->io.data[t->io.num_data_packets++];
	} else if (t->type == RF24_TRANSFER_READ) {
		if (t->io.num_data_packets >= t->io.num_allocated)
			return -ENOMEM;
		p = &t->io.ackdata[t->io.num_ack_packets++];
	} else
		return -EIO; /* Wrong type of transfer */

	p->pipe = pipe;
	memcpy(p->data, data, len);

	return 0;
}


/* Dongle API */
void librf24_packet_transferred(struct rf24_adaptor *a, struct rf24_packet *p)
{
	struct rf24_transfer *t = a->current_transfer;
	
	if (p == t->io.current_data_packet)
		t->io.num_data_packets_xfered++; 
	else
		t->io.num_ack_packets_xfered++; 

	if (t->io.num_data_packets_xfered == t->io.num_data_packets) {
		/* All payload delivered, wait for system to become ready */
		if (!(t->flags & RF24_TRANSFER_FLAG_SYNC)) { 
			t->status = RF24_TRANSFER_COMPLETED;
			rf24_callback(t);
		}
	}	
}


struct rf24_packet *librf24_request_next_packet(struct rf24_adaptor *a, int isack) 
{
	struct rf24_transfer *t = a->current_transfer; // current
	if (!t)
			return NULL; /* No active transfer */

	if (RF24_TRANSFER_IS_CONF(t))
		return NULL;

	if (t->status != RF24_TRANSFER_RUNNING)
		return NULL;

	if (t->status == RF24_TRANSFER_CANCELLING) { 
		t->status = RF24_TRANSFER_CANCELLED;
		rf24_callback(t);
		return NULL;
	}

	if (transfer_is_timeouted(t))
		return NULL; 

	dbg("type %d request packet space isack: %d, data: %zu/%zu ack %zu/%zu\n", 
	    t->type, isack, t->io.num_data_packets_xfered, t->io.num_data_packets, 
	    t->io.num_ack_packets_xfered, t->io.num_ack_packets);

	struct rf24_packet *ret = NULL;

	if ((!isack) && (t->io.num_data_packets_xfered < t->io.num_data_packets)) {
		ret = &t->io.data[t->io.num_data_packets_xfered];
		t->io.current_data_packet = ret;
	}

	if ((isack) && (t->io.num_ack_packets_xfered < t->io.num_ack_packets)) {
		ret = &t->io.data[t->io.num_ack_packets_xfered];
		t->io.current_ack_packet = ret;
	}

	return ret;
}


void librf24_report_state(struct rf24_adaptor *a, int num_pending, int num_failed)
{
	struct rf24_transfer *t = a->current_transfer; // current
	if (!t) {		
		return;
	}

	if (RF24_TRANSFER_IS_CONF(t))
		return;

	if (t->status == RF24_TRANSFER_CANCELLING) { 
		t->status = RF24_TRANSFER_CANCELLED;
		rf24_callback(t);
	}
	
	if (transfer_is_timeouted(t))
		return; 

	t->io.num_data_packets_failed += num_failed;

	if ((t->io.num_data_packets_xfered == t->io.num_data_packets) &&
	    (t->io.num_ack_packets_xfered == t->io.num_ack_packets) && 
	    (num_pending == 0)) { 
		if (t->io.num_data_packets_failed == 0)
			t->status = RF24_TRANSFER_COMPLETED;
		else
			t->status = RF24_TRANSFER_FAIL;
		rf24_callback(t);
	}
}
