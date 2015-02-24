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
	class LibRF24SweepTransfer: public LibRF24Transfer {
	public:
		LibRF24SweepTransfer(LibRF24Adaptor &a, int times);
		~LibRF24SweepTransfer();
		void clear();
		unsigned int getObserved(int ch);
	protected:
		void handleData(unsigned char *data, size_t size);
		void transferStarted();
		void adaptorNowIdle(bool lastOk);
	private:
		int numTimes;
		unsigned int observed[128];
	};
};
