//封装一个ioserver， work守护#pragma once
#include <vector>
#include <thread>
#include <string>
#include <mutex>
#include <iostream>
#include <boost/serialization/singleton.hpp>
#include <boost/thread/concurrent_queues/sync_queue.hpp>
#include <boost/asio.hpp>

#include "ConnectionImpl.h"

using boost::asio::ip::udp;


namespace bitcraze {
namespace crazyflieLinkCpp {

    using boost_ec = boost::system::error_code;
    using thread_ptr = std::shared_ptr<std::thread>;
    using work_ptr = std::unique_ptr<boost::asio::io_service::work>;
    #define ioService CIoService::get_mutable_instance()

        class CIoService : public boost::serialization::singleton<CIoService>
        {
        public:
            CIoService() : m_work(m_ios) {}

            //多个线程调用io_server事件
            void Run(uint16_t count) {
                std::call_once(m_of,
                    [&]() {
                        for (uint16_t i = 0; i < count; ++i)
                        {
                            std::shared_ptr<std::thread> t(new std::thread([&]() { boost::system::error_code ec;  m_ios.run(ec); }));
                            m_vtr_thread.emplace_back(t);
                        }
                    });
            }
            void Stop() {
                m_ios.stop();
                for (uint16_t i = 0; i < m_vtr_thread.size(); ++i)
                {
                    if (m_vtr_thread[i]->joinable())
                    {
                        m_vtr_thread[i]->join();
                    }
                }
            }
            boost::asio::io_service& GetIoService() { return m_ios; }
        protected:
            boost::asio::io_service m_ios;
            boost::asio::io_service::work m_work;
            std::vector<thread_ptr> m_vtr_thread;
            std::once_flag m_of;
        };


        //封装通信数据包
        struct STUDPPacket
        {
            enum { MAX_DATA_LEN = 1024 };
            unsigned char data[MAX_DATA_LEN] = { 0 };
            size_t len{ 0 };

            std::string src_addr;        //源地址
            std::string dst_addr{"192.168.43.42"};        //目的地址，对于发送的数据包，在这里指定发送ip
            uint16_t src_port{ 0 };        //源端口
            uint16_t dst_port{ 2390 };        //目的端口，对于发送的数据包，在这里指定发送端口

            STUDPPacket()
            {
                memset(data, 0, sizeof(data));
            }
        };
        using STUDPPacketPtr = std::shared_ptr<STUDPPacket>;

        //定义一个 CUDPServer类
        class CUDPServer
        {
            using sync_recv_queue = boost::concurrent::sync_queue<STUDPPacketPtr>;
            using sync_send_queue = boost::concurrent::sync_queue<STUDPPacketPtr>;

        public:
            CUDPServer() { };
            ~CUDPServer()
            {
                ioService.Stop();
                if (m_socket && m_socket->is_open())
                {
                    m_socket->close();
                }
                m_recv_queue.close();
                m_send_queue.close();
                Stop();
            };

            bool send(const uint8_t* data, uint32_t length);
            size_t recv(uint8_t* buffer, size_t max_length, unsigned int timeout);

            boost_ec Start(boost::asio::io_service& ios, std::shared_ptr<ConnectionImpl> con, uint16_t port = 2399);
            
        protected:
            void DoRecv();
            void Stop();

            void HandleRecvPack();
            void HandleSendPack();
        protected:
            udp::endpoint m_sender_endpoint;        //当接收到数据时，填入数据发送端的地址
            std::shared_ptr<udp::socket> m_socket;    //socket绑定本地UDP端口
            sync_recv_queue m_recv_queue;            //UDP数据包接收队列
            sync_recv_queue m_send_queue;            //UDP数据包发送队列
            std::vector<thread_ptr> m_vtr_thread;    //数据发送线程，接收到的数据处理线程
            std::shared_ptr<ConnectionImpl> connection_;
        };

    }
}