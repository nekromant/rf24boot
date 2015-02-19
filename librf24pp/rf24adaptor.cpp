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
	LOG(DEBUG) << "Creating base adaptor";
	updateStatus();
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
	startTransfers();
	return true;
}

bool LibRF24Adaptor::cancel(LibRF24Transfer *t)
{
	if (t->locked) 
		return false; /* We can't cancel transfers that are already somewhere in hardware
				 by ourselves */

	
	std::vector<LibRF24Transfer*>::iterator pos = std::find(queue.begin(), queue.end(), t);
	
	if (pos == queue.end())
		throw std::range_error("Attempt to cancel non-existent transfer");
	t->fireCallback(TRANSFER_CANCELLED);
	queue.erase(pos);


	return true;
}


uint64_t LibRF24Adaptor::currentTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec *(uint64_t) 1000) + (tv.tv_usec / 1000);
}

void LibRF24Adaptor::bufferWrite(LibRF24Packet *pck)
{
	bufferWriteDone(pck); /* Just call the callback */
}


void LibRF24Adaptor::bufferRead(LibRF24Packet *pck)
{
	bufferReadDone(pck);
}


/* All work happens in these callbacks */
void LibRF24Adaptor::bufferReadDone(LibRF24Packet *pck)
{

}

void LibRF24Adaptor::bufferWriteDone(LibRF24Packet *pck)
{
	
}

void LibRF24Adaptor::startTransfers()
{
	bool somethingGoingOn; 
	LibRF24Packet *pck; 

	/* Fair share: do in round-robin fasion, 
	   till we can't write any more 
	*/
	do { 
		somethingGoingOn = false;

		pck = nextForWrite();
		if (pck && countToWrite--) { 
			bufferWrite(pck);
			somethingGoingOn = true;
		}

		pck = nextForRead();
		if (pck && countToRead--) { 
			bufferRead(pck);
			somethingGoingOn = true;
		}
		
	} while (somethingGoingOn);
	
	/* Finally, request a status update */ 
	requestStatus();
}

void LibRF24Adaptor::updateStatus(int newMode, int countCanWrite, int countCanRead, int lastResult)
{
	countToWrite = countCanWrite;
	countToRead  = countCanRead;
	currentMode  = newMode;
	startTransfers();
}

void LibRF24Adaptor::requestStatus()
{
	updateStatus(currentMode, 1, 1, 0);
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
		LOG(DEBUG) << "Looping...";
		loopOnce();
		sleep(1);
	}
}
