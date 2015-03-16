#pragma once

#include <librf24/easylogging++.hpp>
#include <librf24/rf24adaptor.hpp>
#include <librf24/rf24libusbadaptor.hpp>
#include <librf24/rf24transfer.hpp>
#include <librf24/rf24packet.hpp>
#include <librf24/rf24iotransfer.hpp>
#include <librf24/rf24conftransfer.hpp>
#include <librf24/rf24popentransfer.hpp>
#include <librf24/rf24sweeptransfer.hpp>
#include <librf24/rf24address.hpp>

inline std::ostream& operator<<(std::ostream& out, struct librf24::rf24_usb_config currentConfig)
{
	out << "Channel          : " << (int) (currentConfig.channel) << std::endl;
	out << "Rate             : ";
	switch (currentConfig.rate) { 
	case librf24::RF24_2MBPS:
		out << "2 Mbps" << std::endl;
		break;
	case librf24::RF24_1MBPS:
		out << "1 Mbps" << std::endl;
		break;
	case librf24::RF24_250KBPS:
		out << "250 Kbps" << std::endl;
		break;
	};
	out << "Power Ampl       : ";	
	switch (currentConfig.pa) { 
	case librf24::RF24_PA_MIN:
		out << "[|   ], Min" << std::endl;
		break;
	case librf24::RF24_PA_LOW:
		out << "[||  ], Low" << std::endl;
		break;
	case librf24::RF24_PA_HIGH:
		out << "[||| ], High" << std::endl;
		break;
	case librf24::RF24_PA_MAX:
		out << "[||||], Max" << std::endl;
		break;
	default:
		out << "[ ? ],  Error" << std::endl;
		break;
	}

	out << "CRC              : ";	
	switch (currentConfig.crclen) { 
	case librf24::RF24_CRC_DISABLED:
		out << "Disabled" << std::endl;
		break;
	case librf24::RF24_CRC_8:
		out << "8 bit" << std::endl;
		break;
	case librf24::RF24_CRC_16:
		out << "16 bit" << std::endl;
		break;
	}

	out << "Retries          : " << (int) currentConfig.num_retries << std::endl;
	out << "Retry delay      : " << ((int) currentConfig.retry_timeout) * 250 << " uS " << std::endl;
	out << "Dynamic payloads : " << (currentConfig.dynamic_payloads ? "yes" : "no") << std::endl;
	out << "Payload size     : " << (int) currentConfig.payload_size << std::endl;
	out << "Ack payloads     : " << (currentConfig.ack_payloads ? "yes" : "no") << std::endl;
	out << "Auto-ack         : ";
	for (int i=0; i<6; i++) { 
		if (currentConfig.pipe_auto_ack & (1<<i))
			out << "PIPE" << i << " ";
	}
	
	out <<std::endl;
/*
	currentConfig.pipe_auto_ack    = 0xff; 
*/	
	return out;
}
