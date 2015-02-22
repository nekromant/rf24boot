#include <vector>
#include <iterator>
#include <stdint.h>
#include <librf24/easylogging++.hpp>
#include <librf24/rf24defs.h>
#include <librf24/rf24transfer.hpp>
#pragma once


namespace librf24 {
	class LibRF24Adaptor;
	class LibRF24Transfer;
	class LibRF24ConfTransfer: public LibRF24Transfer {
	public:
		LibRF24ConfTransfer(LibRF24Adaptor &a, struct rf24_usb_config *conf);
		~LibRF24ConfTransfer();
	protected:
		void transferStarted();
		void adaptorNowIdle(int lastWriteResult);
	private:
		struct rf24_usb_config curConf;
	};
};
