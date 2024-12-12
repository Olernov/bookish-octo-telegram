#pragma once

#include <boost/asio.hpp>
#include "message.h"
#include "session.h"

// This an asychronous TCP/IP server responsible for accepting incoming
// connections and spawning `Session` objects for each of them
class Server
{
public:
  Server(boost::asio::io_context& io_context, const tcp::endpoint& endpoint)
    : acceptor_(io_context, endpoint)
  {
  }

  void start()
  {
    accept_connections();
  }

  void send(Message&& msg)
  {
    if (current_session_)
    {
      current_session_->send(std::move(msg));
    }
    else
    {
      std::cout << "No active client session to send" << std::endl;
    }
  }

private:
  void accept_connections()
  {
    acceptor_.async_accept(
      [this](boost::system::error_code ec, tcp::socket socket)
      {
        if (!ec)
        {
          std::cout << "\nAccepted incoming connection from "
                    << socket.remote_endpoint().address().to_string()
                    << ":" << socket.remote_endpoint().port() << std::endl;
          current_session_ = std::make_shared<Session>(std::move(socket));
          current_session_->start();
        }
        else
        {
          std::cout << "Error while accepting connection: " << ec.message() << std::endl;
        }

        accept_connections();
      });
  }

  tcp::acceptor acceptor_;
  std::shared_ptr<Session> current_session_;
};
