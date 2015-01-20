#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
#include <librf24/rf24transfer.hpp>

using namespace librf24;


LibRF24Transfer::LibRF24Transfer(LibRF24Adaptor &a) : adaptor(a)
{
	dbg.setPrefix("LibRF24Transfer");	
}

LibRF24Transfer::~LibRF24Transfer()
{
	
}

void LibRF24Transfer::submit() 
{

	checkTransferTimeout();
	//adaptor.submit(*this);
}

void LibRF24Transfer::checkTransferTimeout()
{
	
}

void LibRF24Transfer::fireCallback(enum rf24_transfer_status newStatus)
{
	dbg << "Transfer status change: " << currentStatus << " -> " << newStatus  << dbg.endl; 
	this->currentStatus = newStatus;
	if (this->cb != nullptr)
		this->cb(*this, newStatus);
}


uint64_t LibRF24Transfer::currentTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*(uint64_t) 1000000 + tv.tv_usec;
}
