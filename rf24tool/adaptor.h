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

struct rf24_adaptor_status {
	int buf_size;
	int buf_count;
	int ack_size;
	int ack_count;
	int num_failed;
	int clear_to_send;
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
	struct rf24_config *conf; 
	struct rf24_adaptor_status    status;
	int  (*init)(void *s, int argc, char* argv[]);
	void (*usage)(void);
	void (*config)(void *self, struct rf24_config *conf);
	void (*mode)(void *self, int mode);
	void (*open_reading_pipe)(void *self, int pipe, char *addr);
	void (*open_writing_pipe)(void *self, char *addr);
	int  (*write)(void *self, void *buffer, int size);
	int  (*read)(void *self, int *pipe, void *buffer, int size);
	int  (*sync)(void *self, uint16_t timeout);
	void (*flush)(void *self);
	int  (*sweep)(void *self, int times, uint8_t *buffer, int size);

/*  New async API */
	int (*submit_transfer)(struct rf24_transfer *t);
	int (*cancel_transfer)(struct rf24_transfer *t);
	int  (*handle_events)(void *a);
	int  (*allocate_transferdata)(struct rf24_transfer *t);
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

void rf24_mode(struct rf24_adaptor *a, int mode);
void rf24_open_reading_pipe(struct rf24_adaptor *a, int pipe, char *addr);
void rf24_open_writing_pipe(struct rf24_adaptor *a, char *addr);
int rf24_write(struct rf24_adaptor *a, void *buffer, int size);
int rf24_read(struct rf24_adaptor *a, int *pipe, void *buffer, int size);
int rf24_sync(struct rf24_adaptor *a, uint16_t timeout);
void rf24_flush(struct rf24_adaptor *a);
int rf24_init(struct rf24_adaptor *a, int argc, char *argv[]);
int rf24_sweep(struct rf24_adaptor *a, int times, uint8_t *buffer, int size);





/* Async API */
struct rf24_transfer *rf24_allocate_io_transfer       (struct rf24_adaptor *a, int npackets);
struct rf24_transfer *rf24_allocate_sweep_transfer    (struct rf24_adaptor *a, int times);
struct rf24_transfer *rf24_allocate_config_transfer   (struct rf24_adaptor *a, struct rf24_config *conf);
struct rf24_transfer *rf24_allocate_openpipe_transfer (struct rf24_adaptor *a, enum rf24_pipe pipe, char* addr);


int rf24_make_read_transfer(struct rf24_transfer *t, int numpackets);
int rf24_make_write_transfer(struct rf24_transfer *t, int mode);

int rf24_add_packet(struct rf24_transfer *t, char* data, int len, int pipe);

void rf24_set_transfer_timeout(struct rf24_transfer *t, int timeout_ms);
void rf24_set_transfer_callback(struct rf24_transfer *t, rf24_transfer_cb_fn callback);

int rf24_submit_transfer(struct rf24_transfer *t);
void rf24_cancel_transfer(struct rf24_transfer *t);

int rf24_handle_events(struct rf24_adaptor *a);
int rf24_handle_events_completed(struct rf24_adaptor *a, int *completion);

void rf24_callback(struct rf24_transfer *t);

const struct libusb_pollfd** rf24_get_pollfds(struct rf24_adaptor *a); 	
