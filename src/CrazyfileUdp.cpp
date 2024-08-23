#include "CrazyfileUdp.h"

#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <memory>

#include <boost/asio.hpp>
#include<boost/date_time/posix_time/posix_time.hpp>

#define __DEBUG__  std::cout /*<<__FILE__*/<< " " << __FUNCTION__<< " "<<__LINE__<< " " << boost::posix_time::to_iso_string(boost::posix_time::microsec_clock::local_time()) << " "

namespace bitcraze {
namespace crazyflieLinkCpp {

std::string CrazyfileUdp::toHex(const unsigned char* data, int len)
{
	std::stringstream ss;
	ss << std::uppercase << std::hex << std::setfill('0');
	for (int i = 0; i < len; i++) {
		ss << std::setw(2) << static_cast<unsigned>(data[i]);
	}
	return ss.str();
}

void CrazyfileUdp::keepLive()
{
#if 1
	//str1=b'\xFF\x01\x01\x01'
	unsigned char data[4] = { 0xFF, 0x01, 0x01, 0x01 };
	try {
		auto it = socket_.send_to(boost::asio::buffer(data), flightEndPoint_);
		//__DEBUG__ << " send size:" << it << " data:" << toHex(data, 4) << std::endl;
	}
	catch (...)
	{
		__DEBUG__ << " send data error" << std::endl;
	}
#endif
}


CrazyfileUdp::CrazyfileUdp(boost::asio::io_context& io, const std::string& ip, uint16_t dstPort, uint16_t localPort)
	:ip_{ ip }, resolver_{io}, dstPort_{ dstPort }, localPort_{ localPort }
	, flightEndPoint_{ *resolver_.resolve(udp::v4(), ip, std::to_string(dstPort)).begin()}
	, pcEndpoint_{ udp::endpoint(udp::v4(), localPort) }
	,socket_{ io }
{
	boost::system::error_code ec;
	boost::asio::ip::address listen_addr = boost::asio::ip::address::from_string("192.168.43.43", ec);
	pcEndpoint_ = boost::asio::ip::udp::endpoint(listen_addr, localPort_);
	socket_.open(pcEndpoint_.protocol(), ec); // boost::asio::ip::udp::socket
	if (ec)
	{
		std::stringstream sstr;
		sstr << "Open 192.168.43.43 error: " << ec.message();
		__DEBUG__ << sstr.str() << std::endl;
		//throw std::runtime_error(sstr.str());
		return;
	}
	socket_.set_option(boost::asio::ip::udp::socket::reuse_address(true), ec);
	if (ec)
	{
		std::stringstream sstr;
		sstr << "set option: reuse address(192.168.43.43) error: " << ec.message();
		__DEBUG__ << sstr.str() << std::endl;
		//throw std::runtime_error(sstr.str());
		return;
	}
	socket_.bind(pcEndpoint_, ec);
	if (ec)
	{
		std::stringstream sstr;
		sstr << "bind 192.168.43.43 error: " << ec.message();
		__DEBUG__ << sstr.str() << std::endl;
		//throw std::runtime_error(sstr.str());
		return;
	}

#if 1
	//str1=b'\xFF\x01\x01\x01'
	unsigned char data[4] = { 0xFF, 0x01, 0x01, 0x01 };
	try {
		auto it = socket_.send_to(boost::asio::buffer(data), flightEndPoint_);
		//__DEBUG__ << " send size:" <<  it <<" data:" << toHex(data, 4) << std::endl;
	}
	catch (...)
	{
		__DEBUG__ << " send data error" << std::endl;
	}
#endif
}

CrazyfileUdp::~CrazyfileUdp()
{
	if (socket_.is_open())
	{
		//socket_.cancel();
		socket_.close();
		__DEBUG__ << "close socket" << std::endl;
	}
	else
	{
		__DEBUG__ << "not open socket" << std::endl;
	}
}

bool CrazyfileUdp::send(const uint8_t* data, uint32_t length)
{
	std::unique_ptr<uint8_t[]> raw = std::make_unique<uint8_t[]>(length + 1);
	uint64_t checkSum = 0;
	for (uint32_t i = 0; i < length; ++i)
	{
		checkSum += data[i];
		raw[i] = data[i];
	}
	//__DEBUG__ << "raw data:"<< toHex(data, length) << std::endl;
	checkSum = checkSum % 256;
	raw[length] = uint8_t(checkSum);

	size_t transferred = socket_.send_to(boost::asio::buffer(raw.get(), length + 1), flightEndPoint_);
	return transferred == length + 1;
	/*if (transferred == length + 1)
	{
		//std::string str = toHex(raw.get(), length + 1);
		//std::cout << " send data:" << str << " size:" << transferred << std::endl;
		return true;
	}
	else if (transferred > 0) {
		// Fixing partial transfers would require a protocol change, so throw an exception instead.
		std::stringstream sstr;
		sstr << "Timeout, but already transferred " << transferred << " of " << length << " bytes!";
		throw std::runtime_error(sstr.str());
	}
	else
	{
		std::string str = toHex(raw.get(), length + 1);
		__DEBUG__ << " ERROR send data error:" << str << " size:" << transferred << std::endl;
	}
	return false;*/
}

size_t CrazyfileUdp::recv(uint8_t* buffer, size_t max_length, unsigned int timeout)
{
	size_t recvCount = 0; 
	if (timeout == 0)
	{
		//__DEBUG__ << "recv length:" << max_length << std::endl;
		//__DEBUG__ << "recv to " << flightEndPoint_.address() << " port:" << flightEndPoint_.port() << std::endl;
		recvCount  = socket_.receive_from(boost::asio::buffer(buffer, max_length), flightEndPoint_);
		if (++keepLive_ > 10)
		{
			keepLive();
			keepLive_ = 0;
		}
		//__DEBUG__ << "recv length:" << recvCount << " max length:" << max_length << std::endl;
		std::string str = toHex(buffer, recvCount);
		//__DEBUG__ << " recv data:" << str << std::endl;

		if (recvCount == 0)
		{
			return 0;
		}

		uint64_t checkSum = 0;
		uint64_t cksum_recv = buffer[recvCount - 1];
		for (uint32_t i = 0; i < recvCount - 1; ++i)
		{
			checkSum += buffer[i];
		}
		checkSum %= 256;
		if (checkSum != cksum_recv)
		{
			__DEBUG__ << "recv check sum:" << cksum_recv << " check sum:" << checkSum << std::endl;
			return 0;
		}
		return recvCount - 1;
	}
	// 创建一个deadline_timer
	boost::asio::io_service io_service;
	boost::asio::deadline_timer timer(io_service, boost::posix_time::milliseconds(timeout));
	// 启动异步接收，同时设置超时定时器
	timer.async_wait([&](const boost::system::error_code& ec) {
		if (!ec) {
			socket_.cancel(); // 如果定时器触发，则取消异步操作
			//__DEBUG__ << " timeout:" << timeout << std::endl;
		}
		});

	// 启动异步接收操作，并设置5秒超时
	socket_.async_receive_from(
		boost::asio::buffer(buffer, max_length), flightEndPoint_,
		[&](const boost::system::error_code& ec, std::size_t bytes_recvd)
		{
			if (!ec || ec == boost::asio::error::operation_aborted) {
				recvCount = bytes_recvd;
				//__DEBUG__ << "Received " << bytes_recvd << " bytes" << std::endl;
			}
			else
			{
				__DEBUG__ << "Receive failed: " << ec.message() << std::endl;
			}
			timer.cancel();
			
		});
	//__DEBUG__ << " begin recvCount:" << recvCount << std::endl;
	io_service.run();
	
	if (recvCount == 0)
	{
		return 0;
	}

	uint64_t checkSum = 0;
	uint64_t cksum_recv = buffer[recvCount - 1];
	for (uint32_t i = 0; i < recvCount - 1; ++i)
	{
		checkSum += buffer[i];
	}
	checkSum %= 256;
	if (checkSum != cksum_recv)
	{
		__DEBUG__ << "recv check sum:" << cksum_recv << " check sum:" << checkSum << std::endl;
		return 0;
	}
	return recvCount - 1;
}

void CrazyfileUdp::setChannel(uint8_t channel)
{
	channel_ = channel;
}

} // namespace crazyflieLinkCpp
} // namespace bitcraze