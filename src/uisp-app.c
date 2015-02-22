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

char msg[128]; /* usb response buffer */

#define dbg(fmt, ...) sprintf(msg, fmt, ##__VA_ARGS__)


#if CONFIG_HW_BUFFER_SIZE % 2
#error CONFIG_HW_BUFFER_SIZE must be power of 2
#endif

extern struct rf24     *g_radio;
static uchar            last_rq;
static int              rq_len;
static uint16_t         pos; /* Position in the buffer */



/* Streaming mode flag */
static uint8_t          system_state = MODE_IDLE; 
static uint8_t          writing; /* Is there currently a write started ? */


static struct rf_packet pcks[CONFIG_HW_BUFFER_SIZE]; /* Our packet buffer pool */
static struct rf_packet_buffer cb = {
	.size = 0,
	.elems = pcks,
}; 

/* cb that stores ack packets */
static struct rf_packet_buffer acb = {
	.size = 0,
	.elems = pcks,
}; 

#include <lib/nRF24L01.h>

static struct rf24_dongle_status status; 

void post_interrupt_message()
{
	status.cb_count  = cb.count;
	status.cb_size   = cb.size;
	status.acb_count = acb.count;
	status.acb_size  = acb.size;
	status.fifo_is_empty = rf24_tx_empty(g_radio);
	usbSetInterrupt((unsigned char *)&status, sizeof(struct rf24_dongle_status));	
}

int rf24_write_done(struct rf24 *r)
{
	uint8_t status;
	status = rf24_get_status(r);
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

	if (system_state == MODE_READ) {
		while ((!cb_is_full(&cb)) && rf24_available(g_radio, &pipe)) {
			struct rf_packet *p = cb_get_slot(&cb);
			p->len = rf24_get_dynamic_payload_size(g_radio);
			p->pipe = pipe;
			rf24_read(g_radio, p->payload, p->len);
		}	
	} else if (system_state == MODE_WRITE_STREAM) {
		if ((writing) && rf24_write_done(g_radio)) { 
			uint8_t ok, rdy;
			writing = 0;
			rf24_what_happened(g_radio, &status.last_tx_failed, &ok, &rdy);

			/* WHY? 
			if (status.last_tx_failed)
				rf24_flush_tx(g_radio);
			*/

			if (rdy) { 
				/* Read the ack payload?? */
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
		int ret=0;
		uint8_t tmp;
		while (ret != -EAGAIN) {
			struct rf_packet *p = cb_peek(&cb);
			if (!p) 
				break;
			ret = rf24_queue_push(g_radio, p->payload, p->len);
			if (ret==0)
				cb_read(&cb);
		}
		/* Retry any transfers */
		rf24_what_happened(g_radio, &tmp, &tmp, &tmp);
	}
}

uchar   usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;
	switch (rq->bRequest) 
	{
	case RQ_MODE:
		cb_flush(&cb);
		cb_flush(&acb);
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
		return 0;
		break;
	case RQ_READ:
	{
		struct rf_packet *p;
		if (system_state == MODE_READ)
			p = cb_read(&cb);
		else
			p = cb_read(&acb);

		if (!p) 
			return 0;

		msg[0] = p->pipe;
		memcpy(&msg[1], p->payload, p->len);
		usbMsgPtr = (uint8_t *)msg;
		post_interrupt_message();
	
		return p->len + 1;
	}
	case RQ_SWEEP: 
	{
		struct rf24_sweeper s;
		rf24_sweeper_init(&s, g_radio);
		rf24_sweep(&s, rq->wValue.bytes[0]);
		memcpy(msg, s.values, 128);
		usbMsgPtr = (uint8_t *)msg;
		return 128;
		break;
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
	}
	last_rq = rq->bRequest;
	rq_len = rq->wLength.word;
	/* Hack: Always call usbFunctionWrite (retransfers) */
	pos = 0;


	return USB_NO_MSG;
}


const char l_addr[] = { 0xaa, 0xaa, 0xaa, 0xaa, 0xaa }; 
uchar usbFunctionWrite(uchar *data, uchar len)
{
	/* TODO: We can copy data to the actual cb-buffer on write here.
	   This way we can write packets to cb in bulk.
	*/

	memcpy(&msg[pos], data, len);
	pos+=len;
	
	if (pos != rq_len) 
		return 0; /* Need moar data */
	uint8_t ret = 0;
	switch (last_rq) {
	case RQ_OPEN_PIPE:
		if (msg[0] < 6) {
			rf24_open_reading_pipe(g_radio, msg[0], (uchar *) &msg[1]);
		} else
			rf24_open_writing_pipe(g_radio, (uchar *) &msg[1]);
		break;
	case RQ_CONFIG:
	{
		struct rf24_usb_config *c = (struct rf24_usb_config *) msg;
		rf24_power_up(g_radio);
		rf24_config(g_radio, (struct rf24_config *) c);
		if (c->ack_payloads) { 
			cb.size   = CONFIG_HW_BUFFER_SIZE / 2;
			acb.size  = CONFIG_HW_BUFFER_SIZE / 2;
			cb.elems  = pcks;
			acb.elems = &pcks[CONFIG_HW_BUFFER_SIZE / 2];
		} else { 
			cb.size = CONFIG_HW_BUFFER_SIZE;
			cb.elems  = pcks;
			acb.size  = 0;
		}
		break;
	}
	case RQ_WRITE:
	{
		struct rf_packet *p;
		p = ( system_state == MODE_READ ) ? 
			cb_get_slot(&acb) : 
			cb_get_slot(&cb);
		memcpy(p->payload, &msg[1], pos-1);
		p->pipe = msg[0];
		p->len  = pos-1;
		status.last_tx_failed = 0xff; /* In progress */
		post_interrupt_message();
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
	rf24_init(g_radio);
  	usbInit();
}

ANTARES_APP(usb_app)
{
	usbPoll();
	process_radio_transfers();
	if (usbInterruptIsReady())
		post_interrupt_message();
}

