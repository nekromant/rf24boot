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
	requestStatus();
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

void LibRF24Adaptor::transferCompleted(LibRF24Transfer *t)
{
	LOG(DEBUG) << "Completing transfer!";
	if (currentTransfer != t) { 
		throw std::runtime_error("Internal bug: Attempt to complete transfer not current");
	}
	currentTransfer = nullptr;
}

bool LibRF24Adaptor::cancel(LibRF24Transfer *t)
{
	/* Base implementation deals only with queued transfers */
	if (t->status() != TRANSFER_QUEUED) 
		return false; 

	
	std::vector<LibRF24Transfer*>::iterator pos = std::find(queue.begin(), queue.end(), t);
	
	if (pos == queue.end())
		throw std::range_error("Attempt to cancel transfer not in adaptor queue");

	queue.erase(pos);
	t->updateStatus(TRANSFER_CANCELLED, true);
	return true;
}


uint64_t LibRF24Adaptor::currentTime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec *(uint64_t) 1000) + (tv.tv_usec / 1000);
}

void LibRF24Adaptor::bufferWrite(LibRF24Packet *pck)
{
	bufferWriteDone(pck);
}


void LibRF24Adaptor::bufferRead(LibRF24Packet *pck)
{
	bufferReadDone(pck);
}



/* All work happens in these callbacks */
void LibRF24Adaptor::bufferReadDone(LibRF24Packet *pck)
{
	currentTransfer->bufferReadDone(pck);
	numIosPending--;
	/* Request a status update only if we have anything to do and there are no ops in background */ 
	if ((numIosPending == 0) && (currentTransfer != nullptr)) 
		requestStatus();
}

void LibRF24Adaptor::bufferWriteDone(LibRF24Packet *pck)
{
	currentTransfer->bufferWriteDone(pck);
	numIosPending--;
	/* Request a status update only if we have anything to do and there are no ops in background */ 
	if ((numIosPending == 0) && (currentTransfer != nullptr)) 
		requestStatus();
}

void LibRF24Adaptor::updateIdleStatus(int lastTx)
{
	currentTransfer->adaptorNowIdle(lastTx);
}

void LibRF24Adaptor::startTransfers()
{
	if ((currentTransfer == nullptr) && (!queue.empty())) { 
		currentTransfer = *queue.begin();
		queue.erase(queue.begin());
		currentTransfer->transferStarted();
	}
	
	if (currentTransfer != nullptr)
	{
		if ((currentTransfer->transferMode != MODE_ANY) && 
		    (currentTransfer->transferMode != currentMode)) { 
			switchMode(currentTransfer->transferMode);
			return;
		}

		/* Fair share: do in round-robin fasion, 
		   till we can't write/read any more 
		*/
		
		bool somethingGoingOn; 
		LibRF24Packet *pck; 
		if (canXfer) { 
			do { 
				somethingGoingOn = false;
				
				pck = currentTransfer->nextForWrite();
				if (pck && countToWrite--) { 
					bufferWrite(pck);
					somethingGoingOn = true;
					numIosPending++;
				}
				
				
				if ((countToRead) && 
				    (pck = currentTransfer->nextForRead())) 
				{ 
					bufferRead(pck);
					somethingGoingOn = true;
					numIosPending++;
					countToRead--;
				}
				
			} while (somethingGoingOn);
		}
	}

	/* Request a status update only if we have anything to do and there are no ops in background */ 
	if ((numIosPending == 0) && (currentTransfer != nullptr)) 
		requestStatus();
}

void LibRF24Adaptor::updateStatus(int countCanWrite, int countCanRead)
{
	countToWrite = countCanWrite;
	countToRead  = countCanRead;
	LOG(DEBUG) << "canRead: " << countCanRead << " canWrite: " << countCanWrite;
	startTransfers();
}


void LibRF24Adaptor::requestStatus()
{
	updateStatus(1, 1);
}

void LibRF24Adaptor::configureStart(struct rf24_usb_config *conf)
{
	/* Configure inhibints any packet queueing, since cb may change */
	canXfer = false;
	countToRead  = 0;
	countToWrite = 0;
}

void LibRF24Adaptor::configureDone()
{
	canXfer = true;
	requestStatus();
}


void LibRF24Adaptor::switchMode(enum rf24_mode mode)
{
	canXfer = false;
	countToRead  = 0;
	countToWrite = 0;
}

void LibRF24Adaptor::pipeOpenDone() 
{
	canXfer = true;
}

void LibRF24Adaptor::pipeOpenStart(enum rf24_pipe pipe, unsigned char addr[5])
{
	canXfer = false; 
}

void LibRF24Adaptor::switchModeDone(enum rf24_mode newMode)
{
	canXfer = true;
	currentMode = newMode;
	requestStatus();
}


void LibRF24Adaptor::loopOnce()
{
	/* Check all transfers, if any are timed out */
	for(std::vector<LibRF24Transfer *>::iterator it = queue.begin(); 
	    it != queue.end(); ++it) {
		if ((*it)->checkTransferTimeout(true))
			queue.erase(it);
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
