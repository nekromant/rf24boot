#include <stdexcept>
#include <librf24/rf24adaptor.hpp>
#include <librf24/rf24transfer.hpp>
#include <librf24/rf24conftransfer.hpp>
#include <getopt.h>

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

void LibRF24Adaptor::updateIdleStatus(bool lastTx)
{
	if (currentTransfer != nullptr)
		currentTransfer->adaptorNowIdle(lastTx);
}

void LibRF24Adaptor::startTransfers()
{

	if ((currentTransfer == nullptr) && (!queue.empty())) { 
		currentTransfer = *queue.begin();
		queue.erase(queue.begin());
		currentTransfer->transferStarted();
		if (currentTransfer == nullptr) { /* Is it already completed ? */
			requestStatus();
			return;
		}
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
				
				if (countToWrite-- && (pck = currentTransfer->nextForWrite())) {
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
	numIosPending++;
	countToRead  = 0;
	countToWrite = 0;
}

void LibRF24Adaptor::configureDone()
{
	canXfer = true;
	numIosPending--;
	requestStatus();
}

void LibRF24Adaptor::sweepDone(unsigned char results[128])
{
	canXfer = true;
	numIosPending++;
	currentTransfer->handleData(results, 128);
	requestStatus();

	
}

void LibRF24Adaptor::sweepStart(int times)
{
	canXfer = false;
	numIosPending++;
}

void LibRF24Adaptor::switchMode(enum rf24_mode mode)
{
	canXfer = false;
	numIosPending++;
	countToRead  = 0;
	countToWrite = 0;
}

void LibRF24Adaptor::pipeOpenDone() 
{
	numIosPending--;
	canXfer = true;
	requestStatus();
}

void LibRF24Adaptor::pipeOpenStart(enum rf24_pipe pipe, unsigned char addr[5])
{
	numIosPending++;
	canXfer = false; 
}

void LibRF24Adaptor::switchModeDone(enum rf24_mode newMode)
{
	canXfer = true;
	numIosPending--;
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


enum rf24_transfer_status LibRF24Adaptor::configureFromArgs(int argc, const char **argv)
{
	int pa = -1; 
	int rate = -1;
	int channel = -1;
	int rez;

	/* Some sane defaults */
	struct rf24_usb_config conf;
	conf.channel          = 2;
	conf.rate             = RF24_2MBPS;
	conf.pa               = RF24_PA_MAX;
	conf.crclen           = RF24_CRC_16;
	conf.num_retries      = 15;
	conf.retry_timeout    = 15;
	conf.dynamic_payloads = 1;
	conf.payload_size     = 32;
	conf.ack_payloads     = 0;
	conf.pipe_auto_ack    = 0xff; 

	const char* short_options = "c:";
	const struct option long_options[] = {
		{"channel",   required_argument,  NULL,        'c'          },
		{"pa-min",    no_argument,        &pa,         RF24_PA_MIN  },
		{"pa-low",    no_argument,        &pa,         RF24_PA_LOW  },
		{"pa-high",   no_argument,        &pa,         RF24_PA_HIGH },
		{"pa-max",    no_argument,        &pa,         RF24_PA_MAX  },
		{"rate-2m",   no_argument,        &rate,       RF24_2MBPS   },
		{"rate-1m",   no_argument,        &rate,       RF24_1MBPS   },
		{"rate-250k", no_argument,        &rate,       RF24_250KBPS },
		{NULL, 0, NULL, 0}
	};

	int preverr = opterr;
	opterr=0;
	while ((rez=getopt_long(argc, (char* const*) argv, short_options,
				long_options, NULL))!=-1)
	{
		switch (rez) { 
		case 'c':
			channel = atoi(optarg);
			break;
		default:
			break; 
		}
	}

	if (pa != -1)
		conf.pa = pa; 
	if (rate != -1)
		conf.rate = rate;
	if (channel != -1)
		conf.channel = channel;
	
	opterr = preverr;
	LibRF24ConfTransfer ct(*this, &conf);
	return ct.execute();
}
