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

void run_test(const char* port)
{
    ipc::client client;
    if(!client.connect(port)) {
      abort();
    }
    std::vector<ipc::Message> messages;
    const size_t ITERATIONS = 10000;
    auto start = std::chrono::steady_clock::now();

    for(size_t i=0; i < ITERATIONS; ++i) {
        char request[max_length];

        for(size_t index=0; index < max_length; ++index) {
            request[index] = i % 16;
        }
        client.send_message(request, max_length);
        auto start = std::chrono::steady_clock::now();
        client.get_messages(messages, 500);
        auto end = std::chrono::steady_clock::now();

        if(messages.empty()) {
            std::cerr << "No response on iteration " << i << std::endl;
            abort();
        }

        char* buf = (char*)messages[0].address;

        for(size_t index=0; index < max_length; ++index) {
            char c = buf[index];
            if(request[index] != c) {

                std::cerr << (int)request[index] << " != " << (int)buf[index] << std::endl;
                abort();
            }
        }

        messages.clear();
    }

    client.disconnect();
    auto end = std::chrono::steady_clock::now();

    double throughput = (ITERATIONS * max_length) / (double)(end - start).count() * 1e3;
    std::cout << "Throughput for this thread was " << throughput << " MB/s" << std::endl;
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: client_stress_test <port> <threads>\n";
      return 1;
    }

    std::vector<std::thread> threads;
    const size_t THREADS = std::stoi(argv[2]);

    for(size_t i=0; i < THREADS; ++i) {
        threads.push_back(std::thread(std::bind(run_test, argv[1])));
    }

    for(size_t i=0; i < THREADS; ++i) {
        threads[i].join();
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
