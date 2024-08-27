//��װһ��ioserver�� work�ػ�#pragma once
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

            //����̵߳���io_server�¼�
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


        //��װͨ�����ݰ�
        struct STUDPPacket
        {
            enum { MAX_DATA_LEN = 1024 };
            unsigned char data[MAX_DATA_LEN] = { 0 };
            size_t len{ 0 };

            std::string src_addr;        //Դ��ַ
            std::string dst_addr{"192.168.43.42"};        //Ŀ�ĵ�ַ�����ڷ��͵����ݰ���������ָ������ip
            uint16_t src_port{ 0 };        //Դ�˿�
            uint16_t dst_port{ 2390 };        //Ŀ�Ķ˿ڣ����ڷ��͵����ݰ���������ָ�����Ͷ˿�

            STUDPPacket()
            {
                memset(data, 0, sizeof(data));
            }
        };
        using STUDPPacketPtr = std::shared_ptr<STUDPPacket>;

        //����һ�� CUDPServer��
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
            udp::endpoint m_sender_endpoint;        //�����յ�����ʱ���������ݷ��Ͷ˵ĵ�ַ
            std::shared_ptr<udp::socket> m_socket;    //socket�󶨱���UDP�˿�
            sync_recv_queue m_recv_queue;            //UDP���ݰ����ն���
            sync_recv_queue m_send_queue;            //UDP���ݰ����Ͷ���
            std::vector<thread_ptr> m_vtr_thread;    //���ݷ����̣߳����յ������ݴ����߳�
            std::shared_ptr<ConnectionImpl> connection_;
        };

    }
}