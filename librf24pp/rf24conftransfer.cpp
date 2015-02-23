#include <stdint.h>
#include <librf24/rf24adaptor.hpp>
#include <librf24/rf24conftransfer.hpp>


using namespace librf24;

LibRF24ConfTransfer::LibRF24ConfTransfer(LibRF24Adaptor &a, struct rf24_usb_config *conf)
: LibRF24Transfer(a)
{
	curConf = *conf;
}

void LibRF24ConfTransfer::transferStarted()
{
	adaptor.configureStart(&curConf);
}

void LibRF24ConfTransfer::adaptorNowIdle(bool lastOk)
{
	LOG(DEBUG) << "Conf transfer now completed";
	updateStatus(TRANSFER_COMPLETED, true);
}

LibRF24ConfTransfer::~LibRF24ConfTransfer()
{
	
}
