#ifndef REQUESTS_H
#define REQUESTS_H

#ifndef __attribute_packed__
#define __attribute_packed__ __attribute__((packed))
#endif

struct rf24_usb_config {
	unsigned char channel;
	unsigned char pa;
	unsigned char rate;
	unsigned char num_retries;
	unsigned char retry_timeout; 
	unsigned char ack_payloads; 
	unsigned char dynamic_payloads;
	unsigned char payload_size;
	unsigned char crclen;
	unsigned char pipe_auto_ack;
} __attribute_packed__ ;


struct rf24_dongle_status {
	uint8_t cb_count;
	uint8_t cb_size;
	uint8_t acb_count;
	uint8_t acb_size;
	uint8_t last_tx_failed;
	uint8_t fifo_is_empty;
} __attribute_packed__ ;

enum  {
	MODE_IDLE = 0,
	MODE_READ,
	MODE_WRITE_STREAM,
	MODE_WRITE_BULK
};

enum rf24_pipe { 
	PIPE_READ_0=0,
	PIPE_READ_1,
	PIPE_READ_2,
	PIPE_READ_3,
	PIPE_READ_4,
	PIPE_READ_5,
	PIPE_WRITE
};

enum {
	RQ_NOP=0,
	RQ_CONFIG,
	RQ_OPEN_PIPE,
	RQ_MODE,
	RQ_READ,
	RQ_WRITE,
	RQ_SWEEP,
	RQ_POLL,
	RQ_SYNC,
	RQ_FLUSH
};

#endif
