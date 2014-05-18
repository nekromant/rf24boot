#ifndef CB_H
#define CB_H

/* RF packet  buffer. */
struct rf_packet {
	uint8_t payload[32];
	uint8_t len;
};

/* Circular buffer object */
struct rf_packet_buffer {
    int         size;   /* maximum number of elements           */
    int         start;  /* index of oldest element              */
    int         count;    /* index at which to write new element  */
    struct rf_packet  *elems;  /* vector of elements           */
};


void cb_flush(struct rf_packet_buffer *cb);
int cb_is_full(struct rf_packet_buffer *cb);
int cb_is_empty(struct rf_packet_buffer *cb);
struct rf_packet *cb_read(struct rf_packet_buffer *b);
struct rf_packet *cb_peek(struct rf_packet_buffer *b);
struct rf_packet *cb_get_slot(struct rf_packet_buffer *b);

#endif
