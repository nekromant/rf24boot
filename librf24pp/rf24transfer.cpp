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

bool LibRF24Transfer::submit() 
{
	when_started = adaptor.currentTime();
	when_timed_out = when_started + timeout_ms;
	if (checkTransferTimeout(false)) /* incorrect timeout ? */
		return false;
	LOG(DEBUG) << "Transfer started @ " << when_started << " will time out @ " << when_timed_out; 
	adaptor.submit(this);
	return true;
}

bool LibRF24Transfer::checkTransferTimeout(bool finalize)
{
	uint64_t time = adaptor.currentTime();
	timeout_ms_left = timeout_ms - (time - when_started);
	
	if (time > when_timed_out) { 
		if (finalize) 
			fireCallback(TRANSFER_TIMED_OUT);
		return true;
	}
	
	return false;
}

void LibRF24Transfer::fireCallback(enum rf24_transfer_status newStatus)
{
	LOG(DEBUG) << "Transfer status change: " << currentStatus << " -> " << newStatus;
	this->currentStatus = newStatus;
	if (this->cb != nullptr)
		this->cb(*this, newStatus);
}


