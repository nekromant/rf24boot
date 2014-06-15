/*
 *  Universal rf24boot bootloader : uISP App code
 *  Copyright (C) 2014  Andrew 'Necromant' Andrianov <andrew@ncrmnt.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <arch/antares.h>
#include <avr/boot.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <generated/usbconfig.h>
#include <arch/vusb/usbportability.h>
#include <arch/vusb/usbdrv.h>
#include <lib/RF24.h>
#include <string.h>
#include <arch/delay.h>

#include "requests.h"
#include "cb.h"

char msg[128];
#define dbg(fmt, ...) sprintf(msg, fmt, ##__VA_ARGS__)


#if CONFIG_HW_BUFFER_SIZE % 2
#error CONFIG_HW_BUFFER_SIZE must be power of 2
#endif

extern struct rf24     *g_radio;
static uchar            last_rq;
static int              rq_len;
static uint16_t         pos; /* Position in the buffer */


static struct rf_packet pcks[CONFIG_HW_BUFFER_SIZE]; /* Out packet buffer for streaming */

/* Streaming mode flag */
static uint8_t          system_state = MODE_IDLE; 
static uint8_t          writing; /* Is there currently a write started ? */
static uint8_t          fails; /* tx fail counter */


static struct rf_packet_buffer cb = {
	.size = CONFIG_HW_BUFFER_SIZE,
	.elems = pcks,
}; 

#include <lib/nRF24L01.h>

int rf24_write_done(struct rf24 *r)
{
	uint8_t observe_tx, status;
	status = rf24_readout_register(r, OBSERVE_TX, &observe_tx, 1);
	return ((status & ((1<<TX_DS) | (1<<MAX_RT))));
}

void fmt_result(int ret)
{
	strcpy(msg, ((ret==0)) ? "O" : "E");
}

void process_radio_transfers() {
	uint8_t pipe;

	uint8_t count = (cb.count & 0xf) >> 2;
	PORTC &= ~0xf;
	while(count--)
		PORTC |= 1<<count;

	if (system_state == MODE_READ && (!cb_is_full(&cb))) {
		if (rf24_available(g_radio, &pipe)) {
			struct rf_packet *p = cb_get_slot(&cb);
			p->len = rf24_get_dynamic_payload_size(g_radio);
			p->pipe = pipe;
			rf24_read(g_radio, p->payload, p->len);
		}
	} else if (system_state == MODE_WRITE_STREAM) {
		if ((writing) && rf24_write_done(g_radio)) { 
			uint8_t ok,fail,rdy;
			writing = 0;
			rf24_what_happened(g_radio, &ok, &fail, &rdy);
			if (fail) { 
				fails++;
				rf24_flush_tx(g_radio);
			}
			cb_read(&cb);
		}
		if (!writing) {
			struct rf_packet *p = cb_peek(&cb);
			if (p) {
				rf24_start_write(g_radio, p->payload, p->len);
				writing = 1;
			}
		}
	} else if (system_state == MODE_WRITE_BULK) {
		struct rf_packet *p = cb_peek(&cb);
		int ret; 
		if (p) {
			ret = rf24_queue_push(g_radio, p->payload, p->len);
			if (ret==0)
				cb_read(&cb);
		}
	}	
}

uchar   usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;
	switch (rq->bRequest) 
	{
	case RQ_MODE:
		cb_flush(&cb);
		rf24_stop_listening(g_radio);	
		rf24_flush_rx(g_radio); 
		rf24_flush_tx(g_radio); 
		system_state = rq->wValue.bytes[0];

		if (rq->wValue.bytes[0] == MODE_IDLE) 
			rf24_power_down(g_radio);
		else 
			rf24_power_up(g_radio);
		
		if (rq->wValue.bytes[0] == MODE_READ)
			rf24_start_listening(g_radio);
		

		writing = 0;
		fails = 0;
		return 0;
		break;
	case RQ_READ:
	{
		if (!cb.count)
			return 0;
		struct rf_packet *p = cb_read(&cb);
		msg[0] = p->pipe;
		memcpy(&msg[1], p->payload, p->len);
		usbMsgPtr = (uint8_t *)msg;
		return p->len + 1;
	}
	case RQ_SWEEP: 
	{
		rf24_init(g_radio);
		struct rf24_sweeper s;
		rf24_set_data_rate(g_radio, RF24_2MBPS);
		rf24_set_pa_level(g_radio,  RF24_PA_MAX);
		rf24_sweeper_init(&s, g_radio);
		rf24_sweep(&s, rq->wValue.bytes[0]);
		memcpy(msg, s.values, 128);
		usbMsgPtr = (uint8_t *)msg;
		return 128;
		break;
	}
	case RQ_POLL:
	{
		msg[0] = cb.count + writing;
		msg[1] = cb.size;
		msg[2] = fails;
		msg[3] = rf24_queue_empty(g_radio);
		usbMsgPtr = (uint8_t *)msg; 
		return 4; 
	}
	case RQ_FLUSH:
	{
		cb_flush(&cb);
		rf24_stop_listening(g_radio);
		rf24_flush_tx(g_radio);
		rf24_flush_rx(g_radio);
		return 0;
	}
	case RQ_SYNC:
	{
		while (--rq->wValue.word && cb.count) {  
			process_radio_transfers();
			delay_ms(10);
		}
		
		rq->wValue.word = 
			rf24_queue_sync(g_radio, rq->wValue.word);

		uint16_t *ret = (uint16_t *) msg; 
		*ret = rq->wValue.word;
		usbMsgPtr = (uint8_t *)msg; 
		return sizeof(uint16_t);
	}
	case RQ_NOP:
		/* Shit out dbg buffer */
		usbMsgPtr = (uint8_t *)msg;
		return strlen(msg)+1;
		break;
	case RQ_WRITE:
	{ 
		if (rq->wLength.word)
			break;
		uint8_t ret = rf24_write(g_radio, NULL, 0);
		fmt_result(ret);
		return 0;
	}
	}
	last_rq = rq->bRequest;
	rq_len = rq->wLength.word;
	/* Hack: Always call usbFunctionWrite (retransfers) */
	pos = 0;
	return USB_NO_MSG;
}

uchar usbFunctionWrite(uchar *data, uchar len)
{
	memcpy(&msg[pos], data, len);
	pos+=len;
	
	if (pos != rq_len) 
		return 0; /* Need moar data */
	uint8_t ret = 0;
	switch (last_rq) {
	case RQ_OPEN_WPIPE:
		rf24_open_writing_pipe(g_radio, (uchar *) msg);
		break;
	case RQ_OPEN_RPIPE:
		rf24_open_reading_pipe(g_radio, msg[0], (uchar *) &msg[1]);
		break;
	case RQ_CONFIG:
	{
		struct rf24_config *c = (struct rf24_config *) msg;
		rf24_init(g_radio);
		rf24_power_up(g_radio);
		rf24_set_channel(g_radio,   c->channel);
		rf24_set_data_rate(g_radio, c->rate);
		rf24_set_pa_level(g_radio,  c->pa);
		rf24_set_retries(g_radio, c->num_retries, c->retry_timeout);
		rf24_set_crc_length(g_radio, c->crclen);
		if (c->dynamic_payloads)
			rf24_enable_dynamic_payloads(g_radio);
		else
			rf24_set_payload_size(g_radio, c->payload_size);
		if (c->ack_payloads)
			rf24_enable_ack_payload(g_radio);			
		break;
	}
	case RQ_WRITE:
	{
		if (system_state == MODE_WRITE_SINGLE) { 
			ret = rf24_write(g_radio, msg, pos);
			break;
		}
		if (cb_is_full(&cb)) { 
			ret = 1;
			strcpy(msg,"F");
			return 1;
			break; /* Not now! */
		}
		struct rf_packet *p = cb_get_slot(&cb);
		memcpy(p->payload, msg, pos); 
		p->len = pos;
		break;
	}
	}
	fmt_result(ret);
	return 1;
}

void usbReconnect()
{
	DDRD  |=  ((1<<CONFIG_USB_CFG_DPLUS_BIT) | (1<<CONFIG_USB_CFG_DMINUS_BIT)) ;
	_delay_ms(250);
	PORTD &=~ ((1<<CONFIG_USB_CFG_DPLUS_BIT) | (1<<CONFIG_USB_CFG_DMINUS_BIT)) ;
	DDRD  &=~ ((1<<CONFIG_USB_CFG_DPLUS_BIT) | (1<<CONFIG_USB_CFG_DMINUS_BIT)) ;

}

#define ALL_LEDS (1<<2 | 1<< 0 | 1<< 1 | 1<< 3)

#define LED1 (1<<0) 
#define LED2 (1<<1) 
#define LED3 (1<<2) 
#define LED4 (1<<3) 

ANTARES_INIT_LOW(io_init)
{
	DDRC  |=   ALL_LEDS;
	PORTC &= ~ ALL_LEDS;
 	usbReconnect();
	PORTC |=  LED1;
}

ANTARES_INIT_HIGH(usb_init) 
{
  	usbInit();
	rf24_init(g_radio);
}



ANTARES_APP(usb_app)
{
	usbPoll();
	process_radio_transfers();
}

