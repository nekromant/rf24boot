/* Circular buffer example, keeps one slot open */
 
#include <stdio.h>
#include <stdlib.h>
#include "cb.h"

 
 
int cb_is_full(struct rf_packet_buffer *cb) 
{
	return cb->count == cb->size; 
}
 
int cb_is_empty(struct rf_packet_buffer *cb) 
{
	return cb->count == 0; 
}


 
struct rf_packet *cb_read(struct rf_packet_buffer *b)
{
	struct rf_packet *ret = NULL;
	if (!cb_is_empty(b)) {
		ret = &b->elems[b->start];
		b->start = (b->start + 1) % b->size;
		b->count--;	
	}
	return ret;
}

struct rf_packet *cb_peek(struct rf_packet_buffer *b)
{
	struct rf_packet *ret = NULL;
	if (!cb_is_empty(b)) {
		ret = &b->elems[b->start];
	}
	return ret;
}



struct rf_packet *cb_get_slot(struct rf_packet_buffer *b)
{
	if (cb_is_full(b)) {
		cb_read(b); /* drop a packet */
		return cb_get_slot(b);
	}
	int end = (b->start + b->count) % b->size;
	struct rf_packet *ret = &b->elems[end];
	++b->count;	
	return ret;
}

void cb_flush(struct rf_packet_buffer *cb)
{
	cb->count = 0;
}
