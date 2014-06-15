#ifndef ADAPTOR_H
#define ADAPTOR_H

#include "../include/requests.h"


struct rf24_adaptor { 
	char *name;
	int debug;
	struct rf24_config *conf; 
	int (*init)(void *s, int argc, char* argv[]);
	void (*usage)(void);
	void (*config)(void *self, struct rf24_config *conf);
	void (*mode)(void *self, int mode);
	void (*open_reading_pipe)(void *self, int pipe, char *addr);
	void (*open_writing_pipe)(void *self, char *addr);
	void (*set_ack_payload)(void *self, char *payload, int len);
	int  (*write)(void *self, void *buffer, int size);
	int  (*read)(void *self, int *pipe, void *buffer, int size);
	int  (*sync)(void *self, uint16_t timeout);
	void  (*flush)(void *self);
	int  (*sweep)(void *self, int times, uint8_t *buffer, int size);
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
        void (*const fn ## _high []) (void)                             \
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
