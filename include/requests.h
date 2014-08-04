#ifndef REQUESTS_H
#define REQUESTS_H

enum  {
	MODE_IDLE = 0,
	MODE_READ,
	MODE_WRITE_STREAM,
	MODE_WRITE_BULK
};

struct rf24_config {
	unsigned char channel;
	unsigned char pa;
	unsigned char rate;
	unsigned char num_retries;
	unsigned char retry_timeout; 
	unsigned char ack_payloads; 
	unsigned char dynamic_payloads;
	unsigned char payload_size;
	unsigned char crclen;
} __attribute__((packed));

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
