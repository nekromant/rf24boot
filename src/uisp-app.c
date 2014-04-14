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

extern struct rf24 *g_radio;
static uchar last_rq;
static int rq_len;
uint8_t local_addr[5];
uint8_t remote_addr[5];
static uint16_t pos; /* Position in the buffer */


static struct rf_packet pcks[4]; 
static struct rf_packet_buffer cb = {
	.size = 4,
	.elems = pcks,
}; 

enum  {
	STATE_IDLE = 0,
	STATE_READ,
	STATE_WRITE
};

static uint8_t system_state = STATE_IDLE; 

uchar   usbFunctionSetup(uchar data[8])
{
	usbRequest_t    *rq = (void *)data;
	switch (rq->bRequest) 
	{
	case RQ_SET_CONFIG:
		rf24_init(g_radio);
		rf24_enable_dynamic_payloads(g_radio);
		rf24_set_channel(g_radio,   rq->wValue.bytes[0]);
		rf24_set_data_rate(g_radio, rq->wValue.bytes[1]);
		rf24_set_pa_level(g_radio,  rq->wIndex.bytes[0]);
		break;
	case RQ_OPEN_PIPES:
		
		break;
	case RQ_LISTEN:
		cb_flush(&cb);
		rf24_power_up(g_radio);
		if (rq->wValue.bytes[0]) {
			rf24_open_reading_pipe(g_radio, 1, local_addr);
			rf24_start_listening(g_radio);
			system_state = STATE_READ;
		} else {
			rf24_stop_listening(g_radio);
			rf24_open_writing_pipe(g_radio, remote_addr);
			system_state = STATE_WRITE;
		}
		break;
	case RQ_READ:
	{
		struct rf_packet *p = cb_read(&cb);
		if (p) {
			memcpy(msg, p->payload, p->len);
			usbMsgPtr = (uint8_t *)msg;
			return p->len;
		}
		return USB_NO_MSG;	
		break;		
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
		msg[0] = cb.count;
		usbMsgPtr = (uint8_t *)msg; 
		return 1; 
	}
	case RQ_NOP:
		/* Shit out dbg buffer */
		usbMsgPtr = (uint8_t *)msg;
		return strlen(msg)+1;
		break;
	}
	last_rq = rq->bRequest;
	rq_len = rq->wLength.word;
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
	uint8_t retries = 15;
	switch (last_rq) 
	{
	case RQ_SET_REMOTE_ADDR:
		memcpy(remote_addr, msg, 5);
		break;
	case RQ_SET_LOCAL_ADDR:
		memcpy(local_addr, msg, 5);
		break;
	case RQ_WRITE:
		do { 
			rf24_power_up(g_radio);
			rf24_open_writing_pipe(g_radio, remote_addr);	
			ret = rf24_write(g_radio, msg, pos);
			delay_ms(1);
		} while( !ret && retries--);
		break;
	}


	strcpy(msg, ((ret==0) && retries) ? "OK" : "FAIL");
	return 1;
}

void usbReconnect()
{
	DDRD=0xff;
	_delay_ms(250);
	DDRD=0;
}

ANTARES_INIT_LOW(io_init)
{
	DDRC=1<<2 | 1<< 0 | 1<< 1 | 1<< 3;
	PORTC=0xff;
 	usbReconnect();
}

ANTARES_INIT_HIGH(usb_init) 
{
	rf24_init(g_radio);
	rf24_enable_dynamic_payloads(g_radio);
	rf24_set_retries(g_radio, 15, 15);
  	usbInit();
}


ANTARES_APP(usb_app)
{
	usbPoll();
	uint8_t pipe;

	uint8_t count = cb.count & 0xf;
	PORTC&=~0xf;
	PORTC|=count;

	if (system_state == STATE_READ) {
		if (rf24_available(g_radio, &pipe)) {
			int have_moar;
			do { 
				struct rf_packet *p = cb_get_slot(&cb);
				p->len = rf24_get_dynamic_payload_size(g_radio);
				have_moar = !rf24_read(g_radio, p->payload, p->len);
			} while (have_moar);
		}		
	}
	
}
