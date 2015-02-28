#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
#include <stdint.h>
#include <librf24/rf24transfer.hpp>
#include <librf24/rf24adaptor.hpp>

using namespace librf24;


LibRF24Transfer::LibRF24Transfer(LibRF24Adaptor &a) : adaptor(a)
{

}

LibRF24Transfer::~LibRF24Transfer()
{
	
}

enum rf24_transfer_status LibRF24Transfer::status()
{
	return currentStatus;
}


enum rf24_transfer_status LibRF24Transfer::execute()
{
	if (!submit())
		return currentStatus;
	do { 
		adaptor.loopOnce();
	} while ((currentStatus == TRANSFER_QUEUED) || 
		 (currentStatus == TRANSFER_RUNNING));
	return currentStatus;
}

bool LibRF24Transfer::submit() 
{
	when_started = adaptor.currentTime();
	when_timed_out = when_started + timeout_ms;
	currentStatus = TRANSFER_QUEUED;
	if (checkTransferTimeout(false)) /* incorrect timeout ? */
		return false;
	LOG(DEBUG) << "Transfer started @ " << when_started << " will time out @ " << when_timed_out; 
	return adaptor.submit(this);
}

bool LibRF24Transfer::checkTransferTimeout(bool finalize)
{
	uint64_t time = adaptor.currentTime();
	timeout_ms_left = timeout_ms - (time - when_started);
	if (time > when_timed_out) { 
		if (finalize && (currentStatus == TRANSFER_QUEUED)) 
			updateStatus(TRANSFER_TIMED_OUT, true);
		return true;
	}	
	return false;
}

void LibRF24Transfer::updateStatus(enum rf24_transfer_status newStatus, bool callback)
{
	LOG(DEBUG) << "Transfer status change: " << currentStatus << " -> " << newStatus;
	this->currentStatus = newStatus;

	if (callback && (this->cb != nullptr))
		this->cb(*this);
	
	if (callback)
		adaptor.transferCompleted(this);	
}


void LibRF24Transfer::bufferWriteDone(LibRF24Packet *pck)
{
	
}

void LibRF24Transfer::bufferReadDone(LibRF24Packet *pck)
{
	
}

void LibRF24Transfer::adaptorNowIdle(bool lastOk)
{
	
}

LibRF24Packet *LibRF24Transfer::nextForRead()
{
	return nullptr;	
}

LibRF24Packet *LibRF24Transfer::nextForWrite()
{
	return nullptr;
}

