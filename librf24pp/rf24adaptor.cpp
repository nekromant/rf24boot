#include <stdexcept>
#include <librf24/rf24adaptor.hpp>
#include <librf24/rf24transfer.hpp>
#include <sys/time.h>
#include <unistd.h>

using namespace librf24;

typedef std::vector<std::pair<int,short>> pollfdlist;
std::vector<std::pair<int,short>> LibRF24Adaptor::getPollFds() 
{
	pollfdlist dummy;
	return dummy;
}

LibRF24Adaptor::LibRF24Adaptor() 
{
	dbg.setPrefix("LibRF24Adaptor");
	dbg << "Creating base adaptor" << std::endl;
}

LibRF24Adaptor::~LibRF24Adaptor()
{
	
}

bool LibRF24Adaptor::submit(LibRF24Transfer *t)
{
	/* 
	 * Default implementation only queues transfers, 
	 * but doesn't do anything with them 
	 */
	queue.push_back(t);
	return true;
}

bool LibRF24Adaptor::cancel(LibRF24Transfer *t)
{
	if (t->locked) 
		return false; /* We can't cancel transfers that are already somewhere in hardware
				 by ourselves */

/*
	std::vector<LibRF24Transfer>::iterator pos = std::find(queue.begin(), queue.end(), t);

	if (pos == queue.end())
		throw std::range_error("Attempt to cancel non-existent transfer");
	t->fireCallback(TRANSFER_CANCELLED);
	queue.erase(pos);
*/

	return true;
}

uint64_t LibRF24Adaptor::currentTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec *(uint64_t) 1000) + (tv.tv_usec / 1000);
}


void LibRF24Adaptor::loopOnce()
{
	/* Check all transfers, if any are timed out */
	for (LibRF24Transfer *t : queue) {
		if (t->checkTransferTimeout(true))
			queue.erase(queue.begin());
	}
}

void LibRF24Adaptor::loopForever()
{
	while(1) { 
		dbg << "Looping..." << dbg.endl();
		loopOnce();
		sleep(1);
	}
}
