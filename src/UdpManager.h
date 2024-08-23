#pragma once

#include <map>
#include <vector>
#include <mutex>

#include "CrazyfileUdpThread.h"

namespace bitcraze {
namespace crazyflieLinkCpp {

class UdpManager {
public :
    // non construction-copyable
    UdpManager(const UdpManager &) = delete;

    // non copyable
    UdpManager & operator=(const UdpManager &) = delete;

    // deconstruct/cleanup
    ~UdpManager();

    static UdpManager &get()
    {
        static UdpManager instance;
        return instance;
    }

    std::vector<std::string> udpList()const
    {
        std::vector<std::string> udps;
        for (auto const& it : crazyfliesUdp_)
        {
            udps.push_back(it.first);
        }
        return udps;
    }
    size_t numCrazyfliesOverUdp() const {
        return crazyfliesUdp_.size();
    }

    void addConnection(std::shared_ptr<ConnectionImpl> con);

    void removeConnection(std::shared_ptr<ConnectionImpl> con);
    
    void updateDevices();

private :
    UdpManager();

private :
    // udp ip->udp
    std::map<std::string, CrazyfileUdpThread> crazyfliesUdp_;
    std::mutex mutex_;
};

} // namespace crazyflieLinkCpp
} // namespace bitcraze