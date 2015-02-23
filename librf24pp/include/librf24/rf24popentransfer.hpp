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
	class LibRF24PipeOpenTransfer: public LibRF24Transfer {
	public:
		LibRF24PipeOpenTransfer(LibRF24Adaptor &a, enum rf24_pipe pipe, unsigned char addr[5]);
		~LibRF24PipeOpenTransfer();
	protected:
		void transferStarted();
		void adaptorNowIdle(bool lastOk);
	private:
		struct rf24_usb_config curConf;
		unsigned char curAddr[5];
		enum rf24_pipe curPipe;
	};
};
