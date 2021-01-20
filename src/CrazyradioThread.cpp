#include "CrazyradioThread.h"
#include "Crazyradio.h"
#include "Connection.h"

CrazyradioThread::CrazyradioThread(libusb_device *dev)
    : dev_(dev)
{

}

bool CrazyradioThread::isActive() const
{
    return thread_.joinable();
}

void CrazyradioThread::addConnection(Connection *con)
{
    connections_.insert(con);
    if (!isActive()) {
        thread_ = std::thread(&CrazyradioThread::run, this);
    }
}

void CrazyradioThread::removeConnection(Connection *con)
{
    connections_.erase(con);
}

void CrazyradioThread::run()
{
    Crazyradio radio(dev_);

    while(!connections_.empty()) {
        for (auto con : connections_) {
            // reconfigure radio if needed
            if (radio.address() != con->address_)
            {
                radio.setAddress(con->address_);
            }
            if (radio.channel() != con->channel_)
            {
                radio.setChannel(con->channel_);
            }
            if (radio.datarate() != con->datarate_)
            {
                radio.setDatarate(con->datarate_);
            }
            if (!radio.ackEnabled())
            {
                radio.setAckEnabled(true);
            }

            // prepare to send result
            Crazyradio::Ack ack;

            // initialize safelink if needed
            if (con->useSafelink_) {
                if (!con->safelinkInitialized_) {
                    const uint8_t enableSafelink[] = {0xFF, 0x05, 1};
                    auto ack = radio.sendPacket(enableSafelink, sizeof(enableSafelink));
                    if (ack) {
                        con->safelinkInitialized_ = true;
                    }
                } else {
                    // send actual packet via safelink

                    const std::lock_guard<std::mutex> lock(con->queue_send_mutex_);
                    if (!con->queue_send_.empty())
                    {
                        auto p = con->queue_send_.top();
                        p.setSafelink(con->safelinkUp_ << 1 | con->safelinkDown_);
                        ack = radio.sendPacket(p.raw(), p.size() + 1);
                        if (ack && ack.size() > 0 && (ack.data()[0] & 0x04) == (con->safelinkDown_ << 2)) {
                            con->safelinkDown_ = !con->safelinkDown_;
                        }
                        if (ack)
                        {
                            con->safelinkUp_ = !con->safelinkUp_;
                            con->queue_send_.pop();
                        }
                    }

                }
            } else
                {
                    // no safelink
                    const std::lock_guard<std::mutex> lock(con->queue_send_mutex_);
                    if (!con->queue_send_.empty())
                    {
                        const auto p = con->queue_send_.top();
                        ack = radio.sendPacket(p.raw(), p.size() + 1);
                        if (ack)
                        {
                            con->queue_send_.pop();
                        }
                    }
                    else
                    {
                        // TODO: should we send a ping in this case? Make it configurable?
                    }
                }

            // enqueue result
            {
                const std::lock_guard<std::mutex> lock(con->queue_recv_mutex_);
                Packet p_ack(ack.data(), ack.size());
                con->queue_recv_.push(p_ack);
            }
        }
    }
}
