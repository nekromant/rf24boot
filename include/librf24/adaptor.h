#ifndef ADAPTOR_H
#define ADAPTOR_H

#include "../include/requests.h"

struct rf24_transfer;

typedef void (*rf24_transfer_cb_fn)(struct rf24_transfer *transfer);

struct rf24_packet { 
	uint8_t reserved[8]; /* Reserved to avoid memcopying in libusb */
	uint8_t pipe;
	uint8_t data[32];
} __attribute__((packed));

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

enum rf24_transfer_status {
	RF24_TRANSFER_IDLE=0,
	RF24_TRANSFER_RUNNING, 	
	RF24_TRANSFER_COMPLETED, 
	RF24_TRANSFER_FAIL, /* Max retries */
	RF24_TRANSFER_ERROR, /* Hardware fault */
	RF24_TRANSFER_TIMED_OUT, 
	RF24_TRANSFER_CANCELLED,
	RF24_TRANSFER_CANCELLING,
};

enum rf24_transfer_type {
	RF24_TRANSFER_IOCLEAR=0,	
	RF24_TRANSFER_READ, 
	RF24_TRANSFER_WRITE, 
	RF24_TRANSFER_SWEEP, 
	RF24_TRANSFER_CONFIG,
	RF24_TRANSFER_OPENPIPE,
};

enum rf24_queue_mode {
	RF24_QUEUE_TX_STREAM,
	RF24_QUEUE_TX_BULK,
	RF24_QUEUE_RX
};

#define RF24_TRANSFER_FLAG_FREE   1

struct rf24_transfer {
	struct rf24_adaptor          *a;
	enum rf24_transfer_type       type;
	enum rf24_transfer_status     status;

	unsigned int                  flags; 
	unsigned int                  timeout_ms;
	unsigned int                  timeout_ms_left;
	rf24_transfer_cb_fn           callback;
	void                         *userdata; 
	void                         *adaptordata;
	struct rf24_transfer         *next;

	union { 
		/* IO transfer */
		struct { 
			int                           transfermode; 
			size_t                        num_allocated; 
			size_t                        num_data_packets;
			size_t                        num_ack_packets; 
			size_t                        num_data_packets_sent;
			size_t                        num_ack_packets_sent; 
			struct rf24_packet           *data;
			struct rf24_packet           *ackdata; 
		} io ;
		/* Sweep transfer */
		struct { 
			uint8_t                       sweepdata[128];
			int                           sweeptimes;
		} sweep;
		/* config transfer */
		struct rf24_config conf; 
		/* Open pipe transfer */ 
		struct {
			int  pipe;
			char addr[5];
		} pipeopen;
	};
};


struct rf24_adaptor { 
	char *name;
	int debug;
	struct rf24_config            *conf; 

	struct rf24_transfer          *current;
	struct rf24_transfer          *last;

	int  (*allocate_transferdata)(struct rf24_transfer *t);
	int  (*init)(void *s, int argc, char* argv[]);
	void (*usage)(void);

	/* Hardware stuff */
	void (*set_ce)(void *self,  int ce);
	void (*set_csn)(void *self, int csn);
	void (*spi_write)(void *self, unsigned char *data, size_t len);
	void (*spi_read)(void *self, unsigned char *data, size_t len);

	int  (*handle_events)(void *a);
	int  (*handle_events_completed)(void *a, int *completion);
	const struct libusb_pollfd** (*get_pollfds)(struct rf24_adaptor *a); 	
	struct rf24_adaptor *next;
};

void rf24tool_register_adaptor(void *a);
struct rf24_adaptor *rf24_get_adaptor_by_name(char* name);
struct rf24_adaptor *rf24_get_default_adaptor();
void rf24_list_adaptors();


#define TRACE(fmt, ...) if (trace) fprintf(stderr, "ENTER: %s " fmt , __FUNCTION__, ##__VA_ARGS__);

/**
 * Power Amplifier level.
 *
 * For use with setPALevel()
 */
typedef enum { RF24_PA_MIN = 0,RF24_PA_LOW, RF24_PA_HIGH, RF24_PA_MAX, RF24_PA_ERROR } rf24_pa_dbm_e ;

/**
 * Data rate.  How fast data moves through the air.
 *
 * For use with setDataRate()
 */
typedef enum { RF24_1MBPS = 0, RF24_2MBPS, RF24_250KBPS } rf24_datarate_e;

/**
 * CRC Length.  How big (if any) of a CRC is included.
 *
 * For use with setCRCLength()
 */
typedef enum { RF24_CRC_DISABLED = 0, RF24_CRC_8, RF24_CRC_16 } rf24_crclength_e;


#define EXPORT_ADAPTOR(name)						\
        void name ## _export();						\
        void (*const name ## _high []) (void)				\
        __attribute__ ((section (".init_array"), aligned (sizeof (void *)))) = \
        {                                                               \
                &name ## _export					\
        };                                                              \
        void name ## _export() {					\
		rf24tool_register_adaptor(&name);			\
	}
#endif

void rf24_config(struct rf24_adaptor *a, struct rf24_config *conf);


/*
void rf24_mode(struct rf24_adaptor *a, int mode);
void rf24_open_reading_pipe(struct rf24_adaptor *a, int pipe, char *addr);
void rf24_open_writing_pipe(struct rf24_adaptor *a, char *addr);
int rf24_write(struct rf24_adaptor *a, void *buffer, int size);
int rf24_read(struct rf24_adaptor *a, int *pipe, void *buffer, int size);
int rf24_sync(struct rf24_adaptor *a, uint16_t timeout);
void rf24_flush(struct rf24_adaptor *a);
int rf24_init(struct rf24_adaptor *a, int argc, char *argv[]);
int rf24_sweep(struct rf24_adaptor *a, int times, uint8_t *buffer, int size);
*/




/* Async API */
struct rf24_transfer *rf24_allocate_io_transfer       (struct rf24_adaptor *a, int npackets);
struct rf24_transfer *rf24_allocate_sweep_transfer    (struct rf24_adaptor *a, int times);
struct rf24_transfer *rf24_allocate_config_transfer   (struct rf24_adaptor *a, struct rf24_config *conf);
struct rf24_transfer *rf24_allocate_openpipe_transfer (struct rf24_adaptor *a, enum rf24_pipe pipe, char* addr);


int librf24_make_read_transfer(struct rf24_transfer *t, int numpackets);
int librf24_make_write_transfer(struct rf24_transfer *t, int mode);

int librf24_add_packet(struct rf24_transfer *t, char* data, int len, int pipe);

void librf24_set_transfer_timeout(struct rf24_transfer *t, int timeout_ms);
void librf24_set_transfer_callback(struct rf24_transfer *t, rf24_transfer_cb_fn callback);

int librf24_submit_transfer(struct rf24_transfer *t);
void librf24_cancel_transfer(struct rf24_transfer *t);

int librf24_handle_events(struct rf24_adaptor *a);
int librf24_handle_events_completed(struct rf24_adaptor *a, int *completion);

void librf24_callback(struct rf24_transfer *t);

const struct libusb_pollfd** librf24_get_pollfds(struct rf24_adaptor *a); 	
