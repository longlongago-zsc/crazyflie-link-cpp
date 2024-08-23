#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <array>
#include <iostream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
using boost::asio::ip::udp;

#include "ConnectionImpl.h"

namespace bitcraze {
namespace crazyflieLinkCpp {

struct IP
{
	std::string ip_{"192.168.43.42"};
	uint16_t port_{ 2390 };
};

class CrazyfileUdp
{
public:
	explicit CrazyfileUdp(boost::asio::io_context& io, const std::string& ip = "192.168.43.42", uint16_t dstPort = 2390, uint16_t localPort = 2399);
	~CrazyfileUdp();
	bool send(const uint8_t* data,uint32_t length);
	size_t recv(uint8_t* buffer, size_t max_length, unsigned int timeout);

	void setChannel( uint8_t channel);

	uint8_t channel() const {
		return channel_;
	}
	static std::string toHex(const unsigned char* data, int len);

	void keepLive();

private:
	std::string ip_{ "192.168.43.42" };
	udp::resolver resolver_;
	uint16_t dstPort_{ 2390 };
	uint16_t localPort_{ 2399 };
	udp::endpoint flightEndPoint_;
	udp::endpoint pcEndpoint_;
	udp::socket socket_;
	uint8_t channel_;
	int keepLive_{ 0 };
};
}
}