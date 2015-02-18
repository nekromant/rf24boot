#pragma once
#include <vector>

namespace librf24 {
	class LibRF24Transfer;
	class LibRF24Adaptor {
	protected:
		std::vector<LibRF24Transfer *> queue;

	public:
		LibRF24Adaptor();
		~LibRF24Adaptor();
		uint64_t currentTime();		
		virtual bool submit(LibRF24Transfer *t);
		virtual bool cancel(LibRF24Transfer *t);
		virtual void loopOnce();
		virtual void loopForever();
		virtual std::vector<std::pair<int,short>> getPollFds();
		
	private:

	};		
};
