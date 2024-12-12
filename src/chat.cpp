#include <cstdlib>
#include <deque>
#include <iostream>
#include <list>
#include <memory>
#include <set>
#include <utility>
#include "server.h"

using boost::asio::ip::tcp;

void print_usage_and_exit()
{
  std::cerr
    << "Usage: chat mode [host] [port]\n"
    << "  mode    srv|cli      `srv` starts the program in server mode (accepting incoming connections),\n"
    << "                       `cli` - in client mode (program connects to the host)\n"
    << "  host                 IP-address or host name to connect to (only for client mode)\n"
    << "  port                 TCP/IP port used for connection. Default 5401\n";
  exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 2)
    {
      print_usage_and_exit();
    }

    bool is_server_mode = false;
    int port = 5401; // default setting if not given in command line
    if (strcmp(argv[1], "srv") == 0)
    {
      is_server_mode = true;
      if (argc > 2)
      {
        port = atoi(argv[2]);
      }
    }
    else if (strcmp(argv[1], "cli") == 0)
    {
      is_server_mode = false;
      if (argc < 3)
      {
        print_usage_and_exit();
      }
      if (argc > 3)
      {
        port = atoi(argv[3]);
      }
    }
    else
    {
      print_usage_and_exit();
    }

    boost::asio::io_context io_context;
    std::unique_ptr<Server> server;
    std::shared_ptr<Session> session;
    if (is_server_mode)
    {
      tcp::endpoint endpoint(tcp::v4(), port);
      server = std::make_unique<Server>(io_context, endpoint);
      server->start();
      std::cout << "Ready to accept incoming connections" << std::endl;
    }
    else
    {
      // Client mode
      tcp::resolver resolver(io_context);
      boost::system::error_code ec;
      auto endpoints = resolver.resolve(argv[2] /* host name */,
          std::to_string(port), ec);
      tcp::socket socket(io_context);
      std::cout << "Connecting ... " << std::endl;
      boost::asio::connect(socket, endpoints, ec);
      if (ec)
      {
        std::cout << "Unable to connect to host: " << ec.message() << std::endl;
        exit(EXIT_FAILURE);
      }
      session = std::make_shared<Session>(std::move(socket));
      session->start();
      std::cout << "Connected successfully. Ready to send messages" << std::endl;
    }

    std::thread t([&io_context](){ io_context.run(); });
    
    uint32_t msg_id = 1;
    std::string line;
    while (std::getline(std::cin, line))
    {
      if (line.size() > Message::MAX_BODY_SIZE)
      {
        line.resize(Message::MAX_BODY_SIZE);
        std::cout << "  (message was truncated to " << line.size()
                  << " characters)" << std::endl;
      }
      Message msg(Message::Type::text, msg_id, line.size());
      if (msg.body_size() == 0)
      {
        // Sending empty messages does not make sense
        continue;
      }
      std::memcpy(msg.body(), line.data(), msg.body_size());
      msg.encode_header();
      if (is_server_mode)
      {
        server->send(std::move(msg));
      }
      else
      {
        session->send(std::move(msg));
      }
      msg_id++;
    }
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    exit(EXIT_FAILURE);
  }
  return 0;
}
