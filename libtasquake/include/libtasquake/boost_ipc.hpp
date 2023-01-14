#pragma once

#include <deque>
#include <memory>
#include <vector>
#include <boost/asio.hpp>

namespace ipc {

    struct Message {
        void* address = nullptr;
        size_t length = 0;
        size_t connection_id = 0;
    };

    class server;

    class session
    : public std::enable_shared_from_this<session>
    {
    public:
        session(boost::asio::ip::tcp::socket socket, size_t connection_id, server* owner);
        void start();
        boost::asio::ip::tcp::socket socket_;
        size_t connection_id = 0;

    private:  
        void do_read();
        char sockbuf[4096];
        Message m_currentMessage;
        size_t m_uBytesWritten = 0;
        server* owner_ = nullptr;
    };

    class server
    {
    public:
        server() = default;
        void start(short port);
        void stop();
        void delete_session(std::shared_ptr<session> ptr);
        std::shared_ptr<session> get_session(size_t connection_id);
        void get_messages(std::vector<Message>& messages);
        void send_message(size_t connection_id, void* data, uint32_t size);
        void message_from_connection(Message msg);
        void get_sessions(std::vector<size_t>& session_ids);

        boost::asio::io_service* service_ = nullptr;
        boost::asio::ip::tcp::acceptor* acceptor_ = nullptr;
        boost::asio::ip::tcp::socket* socket_ = nullptr;
        bool m_bConnected = false;
    private:
        void do_accept();
        void run_service();

        std::thread m_tThread;
        size_t new_session_id = 0;
        std::vector<std::shared_ptr<session>> sessions_;
        std::vector<Message> messages;
        std::mutex message_mutex;
    };

    class client
    {
    public:
        client();
        ~client();
        void get_messages(std::vector<Message>& messages, size_t timeoutMsec);
        void send_message(void* data, size_t size);
        void do_read();
        bool connect(const char* port);
        bool disconnect();
        
        boost::asio::io_service* io_service = nullptr;
        boost::asio::ip::tcp::socket* socket_ = nullptr;
        bool m_bConnected = false;
    private:
        char sockbuf[1024];
        Message m_currentMessage;
        size_t m_uBytesWritten = 0;
        std::vector<Message> messages_;
        std::mutex message_mutex;
        std::thread receiveThread;
    };
}
