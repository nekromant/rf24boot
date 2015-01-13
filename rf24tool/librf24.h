#ifndef ADAPTOR_H
#define ADAPTOR_H

#include "../include/requests.h"
struct rf24_transfer;
typedef void (*rf24_transfer_cb_fn)(struct rf24_transfer *transfer);

struct rf24_packet { 
	uint8_t reserved[8]; /* reserved for libusb */
	uint8_t pipe;
	uint8_t data[32];
	uint8_t datalen;
} __attribute__((packed));

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

enum rf24_transfer_status {
	RF24_TRANSFER_IDLE=0,
	RF24_TRANSFER_RUNNING, 	
	RF24_TRANSFER_COMPLETED, 
	RF24_TRANSFER_FAIL, /* One or more packets were not delivered */ 
	RF24_TRANSFER_ERROR, /* Hardware fault */
	RF24_TRANSFER_TIMED_OUT, /* Timeout waiting for transfer to complete */
	RF24_TRANSFER_CANCELLED, /* Transfer was cancelled */
	RF24_TRANSFER_CANCELLING, /* Transfer is being cancelled */
};

enum rf24_transfer_type {
	RF24_TRANSFER_IOCLEAR=0,	
	RF24_TRANSFER_SWEEP, 
	RF24_TRANSFER_CONFIG,
	RF24_TRANSFER_OPENPIPE,
	RF24_TRANSFER_MODE,
	RF24_TRANSFER_READ, 
	RF24_TRANSFER_WRITE, 
};

#define RF24_TRANSFER_IS_IO(t)   (t->type >= RF24_TRANSFER_READ)
#define RF24_TRANSFER_IS_CONF(t) (t->type < RF24_TRANSFER_READ)

enum  rf24_mode {
	RF24_MODE_IDLE = 0,
	RF24_MODE_READ,
	RF24_MODE_WRITE_STREAM,
	RF24_MODE_WRITE_BULK,
	RF24_MODE_INVALID
};


struct rf24_adaptor_status {
	int buf_size;
	int buf_count;
	int ack_size;
	int ack_count;
	int num_failed;
	int clear_to_send;
};

#define RF24_TRANSFER_FLAG_FREE   (1<<0)
#define RF24_TRANSFER_FLAG_SYNC   (1<<1)


struct rf24_transfer {
	struct rf24_adaptor          *a;
	enum rf24_transfer_type       type;
	enum rf24_transfer_status     status;
	unsigned int                  flags; 
	unsigned int                  timeout_ms;
	unsigned int                  timeout_ms_left;
	long                          time_started;
 
	rf24_transfer_cb_fn           callback;
	void                         *userdata; 
	void                         *adaptordata;
	/* Linked list of submitted transfers for this adaptor */
	struct rf24_transfer         *next;
	union { 
		/* IO transfer */
		struct { 
			int                           transfermode; 
			size_t                        num_allocated; 
			size_t                        num_data_packets;
			size_t                        num_ack_packets; 
			size_t                        num_data_packets_xfered;
			size_t                        num_data_packets_failed;
			size_t                        num_ack_packets_xfered; 
			struct rf24_packet           *data;
			struct rf24_packet           *ackdata; 
			struct rf24_packet           *current_data_packet;
			struct rf24_packet           *current_ack_packet; 
		} io ;
		/* Sweep transfer */
		struct { 
			uint8_t                       sweepdata[128];
			int                           sweeptimes;
		} sweep;
		/* config transfer */
		struct rf24_usb_config conf; 
		/* Open pipe transfer */ 
		struct {
			int  pipe;
			char addr[5];
		} pipeopen;
		/* Set mode transfer */
		enum rf24_mode mode;
	};
};



struct rf24_adaptor {
	char *name;
	int debug;
	struct rf24_usb_config *conf;
	struct rf24_adaptor_status    status;
	int  (*init)(void *s, int argc, char* argv[]);
	void (*usage)(void);

	/* Submit and cancel basic (config, openpipe, sweep transfers) */
	int (*submit_transfer)(struct rf24_transfer *t);
	int (*cancel_transfer)(struct rf24_transfer *t);
	int  (*handle_events)(struct rf24_adaptor *a);

	int  (*allocate_transferdata)(struct rf24_transfer *t);
	const struct libusb_pollfd** (*get_pollfds)(struct rf24_adaptor *a);
	struct rf24_adaptor *next;
	/* Current transfer linked list head and tail*/
	struct rf24_transfer *current_transfer;
	struct rf24_transfer *last_transfer;
	/* A useful transfer to submit when we need to switch modes */
	struct rf24_transfer *switchmode_transfer;
	/* Current adaptor IO mode */
	int current_mode;
        /* Internal modeswitch transfer */
	struct rf24_transfer modeswitch_transfer; 

};


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

void rf24_config(struct rf24_adaptor *a, struct rf24_usb_config *conf);

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

struct rf24_adaptor *rf24_get_default_adaptor();
struct rf24_adaptor *rf24_get_adaptor_by_name(char* name);
int rf24_init(struct rf24_adaptor *a, int argc, char *argv[]);



/* The nice and shiny async API */
struct rf24_transfer *rf24_allocate_io_transfer       (struct rf24_adaptor *a, int npackets);
struct rf24_transfer *rf24_allocate_sweep_transfer    (struct rf24_adaptor *a, int times);
struct rf24_transfer *rf24_allocate_config_transfer   (struct rf24_adaptor *a, struct rf24_usb_config *conf);
struct rf24_transfer *rf24_allocate_openpipe_transfer (struct rf24_adaptor *a, enum rf24_pipe pipe, const char* addr);
struct rf24_transfer *rf24_allocate_modeswitch_transfer(struct rf24_adaptor *a, enum rf24_mode mode);


int rf24_make_read_transfer(struct rf24_transfer *t, int numpackets);
int rf24_make_write_transfer(struct rf24_transfer *t, int mode);

int rf24_add_packet(struct rf24_transfer *t, char* data, int len, int pipe);

void rf24_set_transfer_timeout(struct rf24_transfer *t, int timeout_ms);
void rf24_set_transfer_callback(struct rf24_transfer *t, rf24_transfer_cb_fn callback);

int rf24_submit_transfer(struct rf24_transfer *t);
int rf24_cancel_transfer(struct rf24_transfer *t);

int rf24_handle_events(struct rf24_adaptor *a);
int rf24_handle_events_completed(struct rf24_adaptor *a, int *completion);

void rf24_callback(struct rf24_transfer *t);

void rf24_free_transfer(struct rf24_transfer *t);

const struct libusb_pollfd** rf24_get_pollfds(struct rf24_adaptor *a); 	
