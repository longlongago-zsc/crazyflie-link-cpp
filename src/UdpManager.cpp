#include "UdpManager.h"

#include <stdexcept>
#include <functional>
#include <iostream>
#include <cassert>
#include <sstream>
#include <map>

#include "ConnectionImpl.h"

namespace bitcraze {
namespace crazyflieLinkCpp {

class ConnectionImpl;

UdpManager::UdpManager()
{
    updateDevices();
}

UdpManager::~UdpManager()
{
    // Fixes a crash where device gets destroyed before it gets unref'ed
    crazyfliesUdp_.clear();
}

void UdpManager::updateDevices()
{
    //IP ip{ "192.168.43.42", 2390 };
    crazyfliesUdp_.emplace(std::make_pair("udp://192.168.43.42", CrazyfileUdpThread(IP{"192.168.43.42", 2390})));
    std::set<std::string> to_eraseUdp;
    for (const auto& iter : crazyfliesUdp_) {
        if (iter.second.hasError()) {
            to_eraseUdp.insert(iter.first);
        }
    }
    for (std::string devid : to_eraseUdp) {
        crazyfliesUdp_.erase(devid);
        std::cout << "rm " << devid << std::endl;
    }
}

void UdpManager::addConnection(std::shared_ptr<ConnectionImpl> connection)
{
    const std::lock_guard<std::mutex> lk(mutex_);

    // ToDo: this is terrible for perf
    // updateDevices();
    if (connection->isUdp_)
    {
        crazyfliesUdp_.at(connection->uri_).addConnection(connection);
        return;
    }
}

void UdpManager::removeConnection(std::shared_ptr<ConnectionImpl> con)
{
    const std::lock_guard<std::mutex> lk(mutex_);
    // std::cout << "rmCon " << con->uri_ << std::endl;
    if (con->isUdp_)
    {
        crazyfliesUdp_.at(con->uri_).removeConnection(con);
    }
}

} // namespace crazyflieLinkCpp
} // namespace bitcraze