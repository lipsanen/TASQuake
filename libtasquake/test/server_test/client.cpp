//
// blocking_tcp_echo_client.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2015 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <cstring>
#include <iostream>
#include "libtasquake/boost_ipc.hpp"

using boost::asio::ip::tcp;

enum { max_length = 1024 };

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 2)
    {
      std::cerr << "Usage: blocking_tcp_echo_client <port>\n";
      return 1;
    }

    ipc::client client;
    if(!client.connect(argv[1])) {
      return 1;
    }
    std::vector<ipc::Message> messages;

    while(true) {
        std::cout << "Enter message: ";
        char request[max_length];
        std::cin.getline(request, max_length);
        size_t request_length = std::strlen(request);
        client.send_message(request, request_length);
        auto start = std::chrono::steady_clock::now();
        client.get_messages(messages, 500);
        auto end = std::chrono::steady_clock::now();
        std::cout << "got reply in " << (end - start).count() / 1e3 << " micros" << std::endl;

        for(auto& msg : messages) {
          std::cout << "Reply is: ";
          std::cout.write((const char*)msg.address, msg.length);
          std::cout << "\n";
          free(msg.address);
        }
        messages.clear();
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
