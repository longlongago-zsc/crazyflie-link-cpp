#pragma once

#include <set>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "CrazyfileUdp.h"

namespace bitcraze {
namespace crazyflieLinkCpp {

// forward declaration
class UdpManager;
class ConnectionImpl;

class CrazyfileUdpThread
{
    friend class UdpManager;
public:
    explicit CrazyfileUdpThread(const IP& ip);
    explicit CrazyfileUdpThread(CrazyfileUdpThread&& other);
    ~CrazyfileUdpThread();

    IP Ip() {
        return ip_;
    }
    bool hasError() const {
        return !runtime_error_.empty();
    }
private:
    void runWithErrorHandler();
    void run();

    void addConnection(std::shared_ptr<ConnectionImpl> con);

    void removeConnection(std::shared_ptr<ConnectionImpl> con);
private:
    IP ip_;

    std::mutex thread_mutex_;
    std::thread thread_;
    volatile bool thread_ending_;

    std::shared_ptr<ConnectionImpl> connection_;
    std::string runtime_error_;
};

} // namespace crazyflieLinkCpp
} // namespace bitcraze