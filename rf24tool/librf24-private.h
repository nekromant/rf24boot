#ifndef LIBRF24_PRIVATE
#define LIBRF24_PRIVATE

#include "librf24.h"

/* These are called by adaptor code to get packet space and report completion */
struct rf24_packet *librf24_request_next_packet(struct rf24_adaptor *a, int isack);
void rf24_packet_transferred(struct rf24_adaptor *a, struct rf24_packet *p);

/* adaptor factory api */
void rf24tool_register_adaptor(void *a);
struct rf24_adaptor *rf24_get_adaptor_by_name(char* name);
struct rf24_adaptor *rf24_get_default_adaptor();
void rf24_list_adaptors();


#define TRACE(fmt, ...) if (trace) fprintf(stderr, "ENTER: %s " fmt , __FUNCTION__, ##__VA_ARGS__);
#endif
