#include <stdexcept>
#include <stdint.h>
#include <librf24/rf24adaptor.hpp>
#include <librf24/rf24sweeptransfer.hpp>


using namespace librf24;

LibRF24SweepTransfer::LibRF24SweepTransfer(LibRF24Adaptor &a, int times)
: LibRF24Transfer(a)
{
	numTimes = times;
	clear();
}

void LibRF24SweepTransfer::transferStarted()
{
	adaptor.sweepStart(numTimes);
}

void LibRF24SweepTransfer::clear()
{
	for (int i=0; i<128; i++)
		observed[i]=0;
	
}


unsigned int LibRF24SweepTransfer::getObserved(int ch)
{
	if (ch >=128) 
		throw std::range_error("channel out of range");

	return observed[ch];
}

void LibRF24SweepTransfer::handleData(unsigned char *result, size_t size)
{
	for (int i=0; i<128; i++) 
		observed[i] += result[i];
			
	updateStatus(TRANSFER_COMPLETED, true);	
}

void LibRF24SweepTransfer::adaptorNowIdle(bool lastOk)
{

}


LibRF24SweepTransfer::~LibRF24SweepTransfer()
{
	
}
