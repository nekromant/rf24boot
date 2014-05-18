#ifndef ADAPTOR_H
#define ADAPTOR_H

struct rf24_adaptor { 
	char *name;
	int (*init)(void *s, int argc, char* argv[]);
	void (*usage)(void);
	void (*rf24_set_rconfig)(void *self, int channel, int rate, int pa);
	void (*rf24_set_econfig)(void *self, int nretries, int timeout, int streamcanfail);
	void (*rf24_set_local_addr)(void *self, char *addr);
	void (*rf24_set_remote_addr)(void *self, char *addr);
	int  (*rf24_write)(void *self, void *buffer, int size);
	int  (*rf24_sweep)(void *self, int times, uint8_t *buffer, int size);
	int  (*rf24_read)(void *self, void *buffer, int size);
	void (*rf24_listen)(void *self, int state, int stream);
	int  (*rf24_wsync)(void *self);
	struct rf24_adaptor *next;
};

void rf24tool_register_adaptor(void *a);
struct rf24_adaptor *rf24_get_adaptor_by_name(char* name);
struct rf24_adaptor *rf24_get_default_adaptor();



#define TRACE(fmt, ...) if (trace) fprintf(stderr, "ENTER: %s " fmt "\n", __FUNCTION__, ##__VA_ARGS__);

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
