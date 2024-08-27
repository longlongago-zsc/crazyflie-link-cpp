#include "BoostUdpWrapper.h"
#include "CrazyfileUdp.h"

//�첽ʵ��udp server
namespace bitcraze {
namespace crazyflieLinkCpp {

    bool CUDPServer::send(const uint8_t* data, uint32_t length)
    {
        {
            uint64_t checkSum = 0;
            STUDPPacketPtr pack = std::make_shared<STUDPPacket>();
            for (uint32_t i = 0; i < length; ++i)
            {
                checkSum += data[i];
                pack->data[i] = data[i];
            }
            //__DEBUG__ << "raw data:"<< toHex(data, length) << std::endl;
            checkSum = checkSum % 256;
            pack->data[length] = (unsigned char)checkSum;
            pack->len = length + 1;

            m_send_queue.push(pack);
        }
        return true;
    }
    size_t CUDPServer::recv(uint8_t* buffer, size_t max_length, unsigned int timeout)
    {
        (void)buffer;
        (void)max_length;
        (void)timeout;
        return size_t();
    }

    //------------------------------------------------------------------------
// ��������: Start
// �� �� ֵ: boost_ec
// ��    ��: boost::asio::io_service& ios
// ��    ��: uint16_t port    -- �˿�
// ��    ��: UDPPackHandlerPtr handler    -- ���ݰ�������
// ˵    ��: UDP�󶨶˿ڣ���������
//------------------------------------------------------------------------
boost_ec CUDPServer::Start(boost::asio::io_service& ios, std::shared_ptr<ConnectionImpl> con , uint16_t port)
{
    (void)port;
    if (m_socket || !con)
        return boost_ec();
    connection_ = con;

    boost_ec ec;
    boost::asio::ip::address listen_addr = boost::asio::ip::address::from_string("192.168.43.43", ec);
    udp::endpoint pcEndpoint_ = { listen_addr, 2399 };
    m_socket.reset(new udp::socket(ios));

    boost::asio::ip::address recv_addr = boost::asio::ip::address::from_string("192.168.43.42", ec);
    m_sender_endpoint = { recv_addr, 2390};

    m_socket->open(pcEndpoint_.protocol(), ec); // boost::asio::ip::udp::socket
    if (ec)
    {
        std::stringstream sstr;
        sstr << "Open 192.168.43.43 error: " << ec.message();
        __DEBUG__ << sstr.str() << std::endl;
        //throw std::runtime_error(sstr.str());
        return boost_ec();
    }
    m_socket->set_option(boost::asio::ip::udp::socket::reuse_address(true), ec);
    if (ec)
    {
        std::stringstream sstr;
        sstr << "set option: reuse address(192.168.43.43) error: " << ec.message();
        __DEBUG__ << sstr.str() << std::endl;
        //throw std::runtime_error(sstr.str());
        return boost_ec();
    }
    m_socket->bind(pcEndpoint_, ec);
    if (ec)
    {
        std::stringstream sstr;
        sstr << "bind 192.168.43.43 error: " << ec.message();
        __DEBUG__ << sstr.str() << std::endl;
        //throw std::runtime_error(sstr.str());
        return boost_ec();
    }

    printf("bind ok, ip: %s, port: %d, msg:%s \r\n",
        pcEndpoint_.address().to_string().c_str(), pcEndpoint_.port(), ec.message().c_str());


    //�첽��������
    DoRecv();

    //ֻ��һ���̸߳���UDP���ݵķ���
    thread_ptr thread_send(new std::thread(&CUDPServer::HandleSendPack, this));
    m_vtr_thread.emplace_back(std::move(thread_send));


    //����߳�ͬʱ������յ�������
    for (int i = 0; i < 1; ++i)
    {
        thread_ptr thread_recv(new std::thread(&CUDPServer::HandleRecvPack, this));
        m_vtr_thread.emplace_back(std::move(thread_recv));
    }
    return boost_ec();
}

void CUDPServer::Stop()
{
    for (int i = 0; i < m_vtr_thread.size(); ++i)
    {
        if (m_vtr_thread[i]->joinable())
        {
            __DEBUG__ << "stop thread id:" << m_vtr_thread[i]->get_id() << std::endl;
            m_vtr_thread[i]->join();
        }
    }
}

//------------------------------------------------------------------------
// ��������: DoRecv
// �� �� ֵ: void
// ˵    ��: �첽��ȡ����
//------------------------------------------------------------------------
void CUDPServer::DoRecv()
{
    STUDPPacketPtr pack(new STUDPPacket);
    m_socket->async_receive_from(
        boost::asio::buffer(pack->data, STUDPPacket::MAX_DATA_LEN), m_sender_endpoint,
        [this, pack](boost::system::error_code ec, std::size_t bytes_recvd)
        {
            std::string relay;
            if (!ec && bytes_recvd > 0)
            {
                pack->len = bytes_recvd;
                pack->src_addr = m_sender_endpoint.address().to_string(ec);
                pack->src_port = m_sender_endpoint.port();

                m_recv_queue.push(pack);
            }
            DoRecv();
        });
}



//------------------------------------------------------------------------
// ��������: HandleRecvPack
// �� �� ֵ: void
// ˵    ��: ������յ���UDP���ݰ�
//------------------------------------------------------------------------
void CUDPServer::HandleRecvPack()
{
    while (true)
    {
        STUDPPacketPtr pack;
        boost::queue_op_status st = m_recv_queue.wait_pull(pack);
        if (st == boost::queue_op_status::closed)
        {
            break;
        }
        if (pack)
        {
            uint64_t checkSum = 0;
            uint64_t cksum_recv = pack->data[pack->len - 1];
            for (uint32_t i = 0; i < pack->len-1; ++i)
            {
                checkSum += pack->data[i];
            }
            checkSum %= 256;
            if (checkSum != cksum_recv)
            {
                __DEBUG__ << "recv check sum:" << cksum_recv << " check sum:" << checkSum << std::endl;
            }
            else
            {
                Packet p_recv;
                memcpy_s(p_recv.raw(), CRTP_MAXSIZE, pack->data, pack->len - 1);
                size_t size = pack->len - 1;
                p_recv.setSize(size);
                if (p_recv) {
                    {
                        const std::lock_guard<std::mutex> lock(connection_->queue_recv_mutex_);
                        p_recv.seq_ = connection_->statistics_.receive_count;
                        //connection_->queue_recv_.emplace(std::move(p_recv));
                        connection_->queue_recv_.push(p_recv);
                        ++connection_->statistics_.receive_count;
                    }
                    connection_->queue_recv_cv_.notify_one();
                    //__DEBUG__ << p_recv << std::endl;
                }
            }
        }
    }
}


//------------------------------------------------------------------------
// ��������: HandleSendPack
// �� �� ֵ: void
// ˵    ��: ����UDP���ݰ�
//------------------------------------------------------------------------
void CUDPServer::HandleSendPack()
{
    while (true)
    {
        STUDPPacketPtr pack;
        boost::queue_op_status st = m_send_queue.wait_pull(pack);
        //static int link_keep_alive = 0;
        if (st == boost::queue_op_status::closed)
        {
            break;
        }

        boost_ec ec;
        /*boost::asio::ip::address addr = boost::asio::ip::address::from_string(pack->dst_addr, ec);
        if (ec)
        {
            continue;
        }*/

        m_socket->send_to(boost::asio::buffer(pack->data, pack->len), m_sender_endpoint/*udp::endpoint(addr, pack->dst_port)*/, 0, ec);
        if (ec)
        {
            printf("send data error, ip: %s, port: %d, msg:%s \r\n",
                pack->dst_addr.c_str(), pack->dst_port, ec.message().c_str());
        }
        /*else
        {
            printf("send data ok, ip: %s, port: %d, msg:%s \r\n",
                pack->dst_addr.c_str(), pack->dst_port, ec.message().c_str());
            __DEBUG__ << "send data:" << CrazyfileUdp::toHex((unsigned char*)pack->data, pack->len) << std::endl;
        }*/
    }
}
    }
}