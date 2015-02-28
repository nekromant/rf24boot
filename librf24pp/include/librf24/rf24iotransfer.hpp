#include <vector>
#include <iterator>
#include <stdint.h>
#include <librf24/easylogging++.hpp>
#include <librf24/rf24defs.h>
#include <librf24/rf24transfer.hpp>
#pragma once


namespace librf24 {
	class LibRF24Adaptor;
	class LibRF24Packet;
	class LibRF24IOTransfer: public LibRF24Transfer {
	public:
		LibRF24IOTransfer(LibRF24Adaptor &a);
		~LibRF24IOTransfer();
		void setSync(bool st); 
		bool submit();
		void clear();
		void clearSendQueue();
		void clearRecvQueue();
		void makeRead(int numToRead);
		void makeWriteBulk(bool sync);
		void makeWriteStream(bool sync);
		void fromPacket(LibRF24Packet *p);
		void fromString(enum rf24_pipe pipe, std::string &buf);
		void fromBuffer(enum rf24_pipe pipe, const char *buf, size_t len);

		void fromString(std::string &buf);
		void fromBuffer(const char *buf, size_t len);

		void appendFromString(enum rf24_pipe pipe, std::string &buf);
		void appendFromBuffer(enum rf24_pipe pipe, const char *buf, size_t len);
		void appendPacket(LibRF24Packet *p);
		inline bool getLastWriteResult() { 
			return lastWriteOk;
		};
	protected:
		std::vector<LibRF24Packet *> sendQueue;
		std::vector<LibRF24Packet *>::iterator nextToSend;
		std::vector<LibRF24Packet *> recvQueue;
		void adaptorNowIdle(bool lastOk);
		void transferStarted();
		LibRF24Packet *nextForRead();
		LibRF24Packet *nextForWrite();
		void bufferWriteDone(LibRF24Packet *pck);
		void bufferReadDone(LibRF24Packet *pck);

	private:
		std::vector<LibRF24Packet *> packetPool;
		bool isSync = false;
		bool lastWriteOk = false;

		int countToRead = true;
	};
}
