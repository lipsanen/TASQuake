//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Modded example of boost asio to use libtasquake wrapper


#include <cstdlib>
#include <mutex>
#include <iostream>
#include <memory>
#include <deque>
#include <utility>
#include "libtasquake/boost_ipc.hpp"

using boost::asio::ip::tcp;

ipc::server* server_ptr;

void busy_loop() {
  std::vector<ipc::Message> messages;
  while(true) {
    std::this_thread::yield();
    server_ptr->get_messages(messages);
    for(auto& msg : messages) {
      server_ptr->send_message(msg.connection_id, msg.address, msg.length);
      free(msg.address);
    }
  }
}

int main(int argc, char* argv[])
{
  try
  {
    ipc::server server(13337);
    server_ptr = &server;
    std::thread t(busy_loop);

    server.service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
