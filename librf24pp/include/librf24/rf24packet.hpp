#include <stdlib.h>
#include <iostream>

#pragma once

#define LIBRF24_MAX_PAYLOAD_LEN  32
#define LIBRF24_MAX_PIPE         6

namespace librf24 {
	
	class LibRF24Packet {
	public:
		LibRF24Packet();
		LibRF24Packet(const char *buffer, size_t len);
		LibRF24Packet(const char *buffer);
		LibRF24Packet(int pipe, const char *buffer, size_t len);
		LibRF24Packet(int pipe, std::string& s);
		LibRF24Packet(std::string& s);

		char operator[](int);
		std::ostream& streamTo(std::ostream& os);

		~LibRF24Packet();

		const char *c_str();
		std::string to_string();
		
	private:
		size_t len;
		int pipe;
		char databytes[LIBRF24_MAX_PAYLOAD_LEN + 1];
	};
}

inline std::ostream& operator<<(std::ostream& out, librf24::LibRF24Packet& p)
{
	return p.streamTo(out);
}
