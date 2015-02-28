#include <stdint.h>
#include <librf24/rf24adaptor.hpp>
#include <librf24/rf24popentransfer.hpp>


using namespace librf24;

LibRF24PipeOpenTransfer::LibRF24PipeOpenTransfer(LibRF24Adaptor &a, enum rf24_pipe pipe, unsigned char addr[5])
: LibRF24Transfer(a)
{
	memcpy(curAddr, addr, 5);
	curPipe = pipe;
	transferMode = MODE_WRITE_BULK;
}

void LibRF24PipeOpenTransfer::transferStarted()
{
	adaptor.pipeOpenStart(curPipe, curAddr);
}

void LibRF24PipeOpenTransfer::adaptorNowIdle(bool lastOk)
{
	LOG(DEBUG) << "pOpen transfer now completed";
	updateStatus(TRANSFER_COMPLETED, true);
}

LibRF24PipeOpenTransfer::~LibRF24PipeOpenTransfer()
{
	
}
