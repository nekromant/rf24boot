#pragma once
#include <stdlib.h>
#include <iostream>
#include <librf24/rf24defs.h>
#include <librf24/rf24libusbadaptor.hpp>


#define LIBRF24_LIBUSB_OVERHEAD  (8+1)
/* 
   [ 8 bytes setup packet | 1 byte pipe number | packet body ]

 */

#define LIBRF24_MAX_PAYLOAD_LEN  32
#define LIBRF24_MAX_PIPE         6

namespace librf24 {
	class LibRF24Adaptor;
	class LibRF24LibUSBAdaptor;
	class LibRF24Packet {
	protected:
		friend LibRF24Adaptor; 
		friend LibRF24LibUSBAdaptor;
		LibRF24Adaptor *owner = nullptr;

	public:
		LibRF24Packet();
		LibRF24Packet(const char *buffer, size_t len);
		LibRF24Packet(const char *buffer);
		LibRF24Packet(enum rf24_pipe pipe, const char *buffer, size_t len);
		LibRF24Packet(enum rf24_pipe pipe, std::string& s);
		LibRF24Packet(std::string& s);
		
		char operator[](int);
		std::ostream& streamTo(std::ostream& os);

		~LibRF24Packet();

		char *c_str();
		char *raw_buffer();
		std::string to_string();

	private:
		size_t len;
		enum rf24_pipe pipe;
		char databytes[LIBRF24_LIBUSB_OVERHEAD + LIBRF24_MAX_PAYLOAD_LEN + 1];

	};
}

inline std::ostream& operator<<(std::ostream& out, librf24::LibRF24Packet& p)
{
	return p.streamTo(out);
}
