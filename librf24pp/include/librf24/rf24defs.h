#ifndef ADAPTOR_H
#define ADAPTOR_H

enum rf24_transfer_status {
        TRANSFER_IDLE=0,
	TRANSFER_QUEUED, 
        TRANSFER_RUNNING,
        TRANSFER_COMPLETED, 
        TRANSFER_FAIL, /* One or more packets were not delivered */ 
        TRANSFER_ERROR, /* Hardware fault */
        TRANSFER_TIMED_OUT, /* Timeout waiting for transfer to complete */
        TRANSFER_CANCELLED, /* Transfer was cancelled */
        TRANSFER_CANCELLING, /* Transfer is being cancelled */
};

enum  rf24_mode {
        MODE_IDLE = 0,
        MODE_READ,
        MODE_WRITE_STREAM,
        MODE_WRITE_BULK,
        MODE_INVALID
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


#endif
