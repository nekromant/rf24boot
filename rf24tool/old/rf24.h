#ifndef RF24_H
#define RF24_H

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

void rf24_set_config(usb_dev_handle *h, int channel, int rate, int pa);
void rf24_set_local_addr(usb_dev_handle *h, uint8_t *addr);
void rf24_set_remote_addr(usb_dev_handle *h,  uint8_t *addr);
int rf24_write(usb_dev_handle *h, uint8_t *buffer, int size);
void rf24_open_pipes(usb_dev_handle *h);
void rf24_listen(usb_dev_handle *h, int state);

#define TRACE(fmt, ...) if (trace) fprintf(stderr, "ENTER: %s " fmt "\n", __FUNCTION__, ##__VA_ARGS__);

#endif
