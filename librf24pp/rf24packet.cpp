#include <librf24/rf24packet.hpp>
#include <cstring>
#include <stdexcept>
#include <iomanip>

using namespace librf24;


LibRF24Packet::LibRF24Packet(): len(0), pipe(PIPE_READ_0) 
{
	std::memset(&this->databytes[LIBRF24_LIBUSB_OVERHEAD], 0x0, LIBRF24_MAX_PAYLOAD_LEN + 1);
}

LibRF24Packet::LibRF24Packet(const char *buffer, size_t len) : pipe(PIPE_READ_0)
{
	if (len >= LIBRF24_MAX_PAYLOAD_LEN)
		throw std::range_error("Attempting to create a packet of more than max payload");
	std::memcpy(&this->databytes[LIBRF24_LIBUSB_OVERHEAD], buffer, len);
	this->len = len;
}

LibRF24Packet::LibRF24Packet(const char *buffer) : pipe(PIPE_READ_0)
{
	int len = std::strlen(buffer);
	if (len >= LIBRF24_MAX_PAYLOAD_LEN)
		throw std::range_error("Attempting to create a packet of more than max payload");
	std::memcpy(&this->databytes[LIBRF24_LIBUSB_OVERHEAD], buffer, len);
	this->len = len;
}



LibRF24Packet::LibRF24Packet(std::string& s) : LibRF24Packet(s.c_str(), s.length()) { }

LibRF24Packet::LibRF24Packet(enum rf24_pipe pipe, std::string& s): LibRF24Packet(pipe, s.c_str(), s.length()) { }

LibRF24Packet::LibRF24Packet(enum rf24_pipe pipe, const char *buffer, size_t len) : LibRF24Packet(buffer, len)
{
	if ((pipe < 0 ) || pipe > LIBRF24_MAX_PIPE)
		throw std::range_error("Invalid pipe number");
	this->pipe = pipe;
}

std::string LibRF24Packet::to_string() 
{
	return std::string(&this->databytes[LIBRF24_LIBUSB_OVERHEAD]);
}

char *LibRF24Packet::c_str() 
{
	return reinterpret_cast<char *>(&this->databytes[LIBRF24_LIBUSB_OVERHEAD]);
}

char *LibRF24Packet::raw_buffer() 
{
	return reinterpret_cast<char *>(this->databytes);
}

char LibRF24Packet::operator[](int n) 
{
	if ((n < 0) ||(n >= (int) len))
		throw std::range_error("Out of range while indexing packet data");
	return this->databytes[LIBRF24_LIBUSB_OVERHEAD + n];
}

std::ostream& LibRF24Packet::streamTo(std::ostream& os)
{
	os << "PIPE: " << this->pipe << " LEN: " << this->len << " | " << std::setw(2);
	for (int i=0; i < (int) this->len; i++) 
		os << std::hex << int(this->databytes[LIBRF24_LIBUSB_OVERHEAD + i]) << " ";
	os << "| ";
	for (int i=0; i < (int) this->len; i++) {
		char c = this->databytes[LIBRF24_LIBUSB_OVERHEAD + i];
		if (isalnum(c)) 
			os << this->databytes[i + LIBRF24_LIBUSB_OVERHEAD];
		else 
			os << ".";
	}
	return os;
}

librf24::LibRF24Packet::~LibRF24Packet()
{
	
}
