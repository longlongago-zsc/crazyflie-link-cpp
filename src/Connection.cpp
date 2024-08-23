#include <regex>
#include <iostream>
#include <future>

#include "crazyflieLinkCpp/Connection.h"
#include "ConnectionImpl.h"

#include "UdpManager.h"

namespace bitcraze {
namespace crazyflieLinkCpp {

Connection::Connection(const std::string &uri)
    : impl_(std::make_shared<ConnectionImpl>())
{
  // Examples:
  // "usb://0" -> connect over USB
  // "radio://0/80/2M/E7E7E7E7E7" -> connect over radio
  // "radio://*/80/2M/E7E7E7E7E7" -> auto-pick radio
  // "radio://*/80/2M/*" -> broadcast/P2P sniffing on channel 80
  // "radiobroadcast://0/80/2M -> broadcast to all crazyflies on channel 80

  const std::regex uri_regex("(usb:\\/\\/(\\d+)|radio:\\/\\/(\\d+|\\*)\\/(\\d+)\\/(250K|1M|2M)\\/([a-fA-F0-9]+|\\*)(\\?[\\w=&]+)?)|(radiobroadcast:\\/\\/(\\d+|\\*)\\/(\\d+)\\/(250K|1M|2M))|(udp:\\/\\/((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?).){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?))");
  //const std::regex uri_regex("(usb:\\/\\/(\\d+)|radio:\\/\\/(\\d+|\\*)\\/(\\d+)\\/(250K|1M|2M)\\/([a-fA-F0-9]+|\\*)(\\?[\\w=&]+)?)|(radiobroadcast:\\/\\/(\\d+|\\*)\\/(\\d+)\\/(250K|1M|2M))");
  std::smatch match;
  if (!std::regex_match(uri, match, uri_regex)) {
    std::stringstream sstr;
    sstr << "Invalid uri (" << uri << ")!";
    throw std::runtime_error(sstr.str());
  }

  impl_->uri_ = uri;
 //  for (size_t i = 0; i < match.size(); ++i) {
 //    std::cout << i << " " << match[i].str() << std::endl;
 //  }
 //  std::cout << match.size() << std::endl;
  if (match[12].matched)
  {
      impl_->useSafelink_ = false;
      impl_->useAutoPing_ = false;
      impl_->useAckFilter_ = false;
      impl_->broadcast_ = false;
      impl_->isRadio_ = false;
      impl_->isUdp_ = true;
      impl_->devid_ = -1;
      std::cout << impl_->uri_ << std::endl;
  }
  else
  {
      std::stringstream sstr;
      sstr << "Invalid uri (" << uri << ")!";
      throw std::runtime_error(sstr.str());
  }
  UdpManager::get().addConnection(impl_);
}

Connection::~Connection()
{
  close();
}

void Connection::close()
{
    if (impl_->isUdp_)
    {
        UdpManager::get().removeConnection(impl_);
    }
  else if (impl_->devid_ >= 0) {
    UdpManager::get().removeConnection(impl_);
  }
}

std::vector<std::string> Connection::scan(uint64_t address)
{
  UdpManager::get().updateDevices();

  std::vector<std::string> result;
  // Crazyflies over UDP
  if (UdpManager::get().numCrazyfliesOverUdp() > 0) {
      auto it = UdpManager::get().udpList();
      result.insert(result.begin(), it.begin(), it.end());
  }
  return result;
}

std::vector<std::string> Connection::scan_selected(const std::vector<std::string> &uris)
{
  std::vector<std::string> result;
  result.push_back("udp://192.168.43.42");
  return result;
}

void Connection::send(const Packet& p)
{
  const std::lock_guard<std::mutex> lock(impl_->queue_send_mutex_);
  if (!impl_->runtime_error_.empty()) {
    throw std::runtime_error(impl_->runtime_error_);
  }

  p.seq_ = impl_->statistics_.enqueued_count;
  impl_->queue_send_.push(p);
  //__DEBUG__ << "queue_send_:" << impl_->queue_send_.size() << std::endl;
  ++impl_->statistics_.enqueued_count;
}

Packet Connection::recv(unsigned int timeout_in_ms)
{
  std::unique_lock<std::mutex> lk(impl_->queue_recv_mutex_);
  if (!impl_->runtime_error_.empty()) {
    throw std::runtime_error(impl_->runtime_error_);
  }
  if (timeout_in_ms == 0) {
    impl_->queue_recv_cv_.wait(lk, [this] { return !impl_->queue_recv_.empty(); });
  } else {
    std::chrono::milliseconds duration(timeout_in_ms);
    impl_->queue_recv_cv_.wait_for(lk, duration, [this] { return !impl_->queue_recv_.empty(); });
  }

  Packet result;
  if (impl_->queue_recv_.empty()) {
    return result;
  } else {
    result = impl_->queue_recv_.top();
    impl_->queue_recv_.pop();
  }
  return result;
}

std::ostream& operator<<(std::ostream& out, const Connection& p)
{
  out <<"Connection(" << p.impl_->uri_;
  out << ")";
  return out;
}

const std::string& Connection::uri() const
{
  return impl_->uri_;
}

Connection::Statistics Connection::statistics()
{
  if (!impl_->runtime_error_.empty()) {
    throw std::runtime_error(impl_->runtime_error_);
  }
  return impl_->statistics_;
}

} // namespace crazyflieLinkCpp
} // namespace bitcraze