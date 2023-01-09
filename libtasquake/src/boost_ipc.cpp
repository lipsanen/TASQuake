#include "libtasquake/boost_ipc.hpp"

using namespace ipc;

ipc::session::session(boost::asio::ip::tcp::socket socket, size_t connection_id, ipc::server* owner)
: socket_(std::move(socket)), connection_id(connection_id), owner_(owner)
 { }
        
void ipc::session::start() {
    do_read();
}

void ipc::session::do_read() {
    auto self(shared_from_this());
    socket_.async_read_some(boost::asio::buffer(sockbuf, sizeof(sockbuf)),
    [this, self](boost::system::error_code ec, std::size_t length)
    {
    if (!ec)
    {
        // Message received
        Message msg;
        msg.address = malloc(length);
        msg.connection_id = this->connection_id;
        msg.length = length;
        memcpy(msg.address, this->sockbuf, length);
        this->owner_->message_from_connection(msg);
        this->do_read();
    }
    else {
        // Connection terminated, delete the session
        self->owner_->delete_session(self);
    }
    });
}



ipc::server::server(short port) : 
    acceptor_(service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
    socket_(service) {
    do_accept();
}   

void ipc::server::delete_session(std::shared_ptr<session> ptr) {
    sessions_.erase(std::remove(sessions_.begin(), sessions_.end(), ptr));
}

std::shared_ptr<session> ipc::server::get_session(size_t connection_id) {
    for(auto ptr : sessions_) {
      if(ptr->connection_id == connection_id) {
        return ptr;
      }
    }

    return nullptr;
}

void ipc::server::get_messages(std::vector<Message>& messages) {
    std::lock_guard<std::mutex> guard(message_mutex);
    messages = this->messages;
    this->messages.clear();
}

void ipc::server::send_message(size_t connection_id, void* data, size_t size) {
    auto sess = get_session(connection_id);
    sess->socket_.write_some(boost::asio::buffer(data, size));
}

void ipc::server::message_from_connection(Message msg) {
    std::lock_guard<std::mutex> guard(message_mutex);
    messages.push_back(msg);
}

void ipc::server::do_accept() {
    acceptor_.async_accept(socket_,
    [this](boost::system::error_code ec)
    {
        if (!ec)
        {
            size_t connection_id = this->new_session_id;
            ++this->new_session_id;
            auto ptr = std::make_shared<session>(std::move(socket_), connection_id, this);
            ptr->start();
            sessions_.push_back(ptr);
        }

        do_accept();
    });
}
