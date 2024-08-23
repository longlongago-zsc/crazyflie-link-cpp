#include "CrazyfileUdpThread.h"

#include <iostream>
#include "ConnectionImpl.h"
#include <boost/asio/io_context.hpp>

namespace bitcraze {
namespace crazyflieLinkCpp {


CrazyfileUdpThread::CrazyfileUdpThread(const IP& ip)
   : ip_(ip)
, thread_ending_(false)
{
}

CrazyfileUdpThread::CrazyfileUdpThread(CrazyfileUdpThread&& other)
{
    const std::lock_guard<std::mutex> lk(other.thread_mutex_);
    ip_ = std::move(other.ip_);
    thread_ = std::move(other.thread_);
    thread_ending_ = std::move(other.thread_ending_);
    connection_ = std::move(other.connection_);
    runtime_error_ = std::move(other.runtime_error_);
}

CrazyfileUdpThread::~CrazyfileUdpThread()
{
    const std::lock_guard<std::mutex> lock(thread_mutex_);
    if (thread_.joinable()) {
        thread_.join();
    }
}

void CrazyfileUdpThread::runWithErrorHandler()
{
    try {
        run();
    }
    catch (const std::runtime_error& error) {
        connection_->runtime_error_ = error.what();
        runtime_error_ = error.what();
    }
    catch (...) {
    }
}

void CrazyfileUdpThread::run()
{
    __DEBUG__ << std::endl;
    boost::asio::io_context io;
    CrazyfileUdp cf(io, ip_.ip_, ip_.port_);
    bool isSend = false;

    while (!thread_ending_)
    {
        std::this_thread::yield();

        {
            const std::lock_guard<std::mutex> lock(connection_->queue_send_mutex_);
            if (!connection_->queue_send_.empty())
            {
                Packet p_send = connection_->queue_send_.top();
                //__DEBUG__ << "send data:" << CrazyfileUdp::toHex(p_send.raw(), p_send.size()) << std::endl;;
                bool success = cf.send(p_send.raw(), p_send.size());
                if (success) {
                    ++connection_->statistics_.sent_count;
                    connection_->queue_send_.pop();
                    isSend = true;
                }
            }
        }

        if (isSend)
        {
            Packet p_recv;
            size_t size = cf.recv(p_recv.raw(), CRTP_MAXSIZE, 0);
            p_recv.setSize(size);

            if (p_recv) {
                {
                    const std::lock_guard<std::mutex> lock(connection_->queue_recv_mutex_);
                    p_recv.seq_ = connection_->statistics_.receive_count;
                    connection_->queue_recv_.push(p_recv);
                    ++connection_->statistics_.receive_count;
                }
                connection_->queue_recv_cv_.notify_one();
                //__DEBUG__ << p_recv << std::endl;
            }
            isSend = false;
        }
    }
    __DEBUG__ << std::endl;
}

void CrazyfileUdpThread::addConnection(std::shared_ptr<ConnectionImpl> con)
{
    const std::lock_guard<std::mutex> lock(thread_mutex_);
    if (!thread_.joinable() && !connection_) {
        connection_ = con;
        thread_ = std::thread(&CrazyfileUdpThread::runWithErrorHandler, this);
    }
    else {
        throw std::runtime_error("Cannot operate more than one connection over UDP!");
    }
}

void CrazyfileUdpThread::removeConnection(std::shared_ptr<ConnectionImpl> con)
{
    if (connection_ != con) {
        throw std::runtime_error("Connection does not belong to this thread!");
    }

    const std::lock_guard<std::mutex> lock(thread_mutex_);
    thread_ending_ = true;
    thread_.join();
    thread_ = std::thread();
    thread_ending_ = false;
    connection_.reset();
}

} // namespace crazyflieLinkCpp
} // namespace bitcraze
