#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include <sys/time.h>
#include <stdint.h>
#include <librf24/rf24transfer.hpp>
#include <librf24/rf24iotransfer.hpp>
#include <librf24/rf24adaptor.hpp>
#include <librf24/rf24packet.hpp>

using namespace librf24;

void LibRF24IOTransfer::setSync(bool st)
{
	isSync = st;
}


LibRF24IOTransfer::LibRF24IOTransfer(LibRF24Adaptor &__a)  :  LibRF24Transfer(__a) 
{
	
}

LibRF24IOTransfer::~LibRF24IOTransfer()
{
	clear();
}

bool LibRF24IOTransfer::submit() 
{
	nextToSend = sendQueue.begin();
	return LibRF24Transfer::submit();
}

void LibRF24IOTransfer::transferStarted()
{
	LOG(DEBUG) << "IO transfer started, queue " << sendQueue.size() << " packets";
}

void LibRF24IOTransfer::fromString(std::string &buf)
{
	fromString(PIPE_WRITE, buf);
}

void LibRF24IOTransfer::fromString(enum rf24_pipe pipe, std::string &buf)
{
	clearSendQueue();
	appendFromString(pipe, buf);
}

void LibRF24IOTransfer::fromBuffer(const char *buf, size_t len)
{
	fromBuffer(PIPE_WRITE, buf, len);
}

void LibRF24IOTransfer::fromBuffer(enum rf24_pipe pipe, const char *buf, size_t len)
{
	clearSendQueue();
	appendFromBuffer(pipe, buf, len);
}

void LibRF24IOTransfer::appendFromString(enum rf24_pipe pipe, std::string &buf)
{

	appendFromBuffer(pipe, buf.c_str(), buf.length());
}

void LibRF24IOTransfer::appendFromBuffer(enum rf24_pipe pipe, const char *buf, size_t len)
{
	while (len) { 
		size_t tocopy = std::min((size_t)LIBRF24_MAX_PAYLOAD_LEN, len);
		LibRF24Packet *pck = new LibRF24Packet(pipe, buf, tocopy); 		
		sendQueue.push_back(pck);
		len-=tocopy;
		buf+=tocopy;
	}	
}

void LibRF24IOTransfer::adaptorNowIdle(bool lastOk)
{
	LOG(DEBUG) << "idle";
	if (isSync && (nextToSend == sendQueue.end()))
	{  
		lastWriteOk = lastOk;
		updateStatus(TRANSFER_COMPLETED, true);
	}
	
	checkTransferTimeout(true);
}

void LibRF24IOTransfer::makeRead(int numToRead)
{
	clear();
	countToRead = numToRead;
	transferMode = MODE_READ;
}

void LibRF24IOTransfer::makeWriteBulk(bool sync)
{
	clear();
	transferMode = MODE_WRITE_BULK;
	isSync = sync; 	
}

void LibRF24IOTransfer::makeWriteStream(bool sync)
{
	clear();
	transferMode = MODE_WRITE_STREAM;
	isSync = sync; 	
}


void LibRF24IOTransfer::bufferReadDone(LibRF24Packet *pck)
{
	recvQueue.push_back(pck);
}

void LibRF24IOTransfer::bufferWriteDone(LibRF24Packet *pck)
{
	if (!isSync && (nextToSend == sendQueue.end()))
		updateStatus(TRANSFER_COMPLETED, true);
	
}

LibRF24Packet *LibRF24IOTransfer::nextForRead()
{
	return new LibRF24Packet();
}

LibRF24Packet *LibRF24IOTransfer::nextForWrite()
{
	if (nextToSend == sendQueue.end())
		return nullptr;

	return *(nextToSend++);
	
}

void LibRF24IOTransfer::clearSendQueue()
{
	if (sendQueue.empty())
		return;

	for(std::vector<LibRF24Packet *>::iterator it = sendQueue.begin(); 
	    it != sendQueue.end(); ++it) {
		delete *it;
		sendQueue.erase(it);
	}	
}

void LibRF24IOTransfer::clearRecvQueue()
{
	if (recvQueue.empty())
		return;

	for(std::vector<LibRF24Packet *>::iterator it = recvQueue.begin(); 
	    it != recvQueue.end(); ++it) {
		delete *it;
		recvQueue.erase(it);
	}
}

void LibRF24IOTransfer::clear()
{
//	clearSendQueue();
//	clearRecvQueue();
}
