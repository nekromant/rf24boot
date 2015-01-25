#include <librf24/rf24packet.hpp>
#include <cstring>
#include <stdexcept>
#include <iomanip>

using namespace librf24;


LibRF24Packet::LibRF24Packet(): pipe(0), len(0)
{
	std::memset(this->databytes, 0x0, LIBRF24_MAX_PAYLOAD_LEN + 1);
}

LibRF24Packet::LibRF24Packet(const char *buffer, size_t len) : pipe(0)
{
	if (len >= LIBRF24_MAX_PAYLOAD_LEN)
		throw std::range_error("Attempting to create a packet of more than max payload");
	std::memcpy(this->databytes, buffer, len);
	this->len = len;
}

LibRF24Packet::LibRF24Packet(const char *buffer) : pipe(0)
{
	int len = std::strlen(buffer);
	if (len >= LIBRF24_MAX_PAYLOAD_LEN)
		throw std::range_error("Attempting to create a packet of more than max payload");
	std::memcpy(this->databytes, buffer, len);
	this->len = len;
}



LibRF24Packet::LibRF24Packet(std::string& s) : LibRF24Packet(s.c_str(), s.length()) { }

LibRF24Packet::LibRF24Packet(int pipe, std::string& s): LibRF24Packet(pipe, s.c_str(), s.length()) { }

LibRF24Packet::LibRF24Packet(int pipe, const char *buffer, size_t len) : LibRF24Packet(buffer, len)
{
	if ((pipe < 0 ) || pipe > LIBRF24_MAX_PIPE)
		throw std::range_error("Invalid pipe number");
	this->pipe = pipe;
}

std::string LibRF24Packet::to_string() 
{
	return std::string(this->databytes);
}

const char *LibRF24Packet::c_str() 
{
	return reinterpret_cast<const char *>(this->databytes);
}

char LibRF24Packet::operator[](int n) 
{
	if ((n < 0) ||(n >= (int) len))
		throw std::range_error("Out of range while indexing packet data");
	return this->databytes[n];
}

std::ostream& LibRF24Packet::streamTo(std::ostream& os)
{
	os << "PIPE: " << this->pipe << " LEN: " << this->len << " | " << std::setw(2);
	for (int i=0; i < (int) this->len; i++) 
		os << std::hex << int(this->databytes[i]) << " ";
	os << "| ";
	for (int i=0; i < (int) this->len; i++) {
		char c = this->databytes[i];
		if (isalnum(c)) 
			os << this->databytes[i];
		else 
			os << ".";
	}
	return os;
}

librf24::LibRF24Packet::~LibRF24Packet()
{
	
}
