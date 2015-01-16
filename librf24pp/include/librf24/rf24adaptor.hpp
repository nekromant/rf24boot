#pragma once
#include <vector>

#include <librf24/rf24debug.hpp>

namespace librf24 {
	class LibRF24Transfer;
	class LibRF24Adaptor : public LibRF24Debuggable {
	private:

	protected:
		std::vector<LibRF24Transfer *> queue;
//		virtual void handlePendingEvents();

	public:
		LibRF24Adaptor();
		~LibRF24Adaptor();
//		void submitTransfer(LibRF24Transfer &t); 
//		void cancelTransfer(LibRF24Transfer &t);
//		void loopOnce(); 
//		void loopForever(); 

		virtual std::vector<std::pair<int,short>> getPollFds();
	private:

	};		
};
