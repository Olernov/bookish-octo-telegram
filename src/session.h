#pragma once

#include <boost/asio.hpp>
#include "message.h"

using boost::asio::ip::tcp;
using std::chrono::high_resolution_clock;

// This class
// - asychrounously processes incoming TCP/IP messages
// -  validates their format
// - prints text mesages received from remote side
// - sends delivery acknowledgements
// - measures and outputs turn-around time between messages and acknowledgements
// - sends outgoing messages to the remote side
class Session : public std::enable_shared_from_this<Session>
{
public:
  Session(tcp::socket socket)
    : socket_(std::move(socket))
  {
  }

  void start()
  {
    wait_for_incoming_message();
  }

  void send(Message&& msg)
  {
    if (!is_active())
    {
      std::cout << "No active session to send" << std::endl;
      return;
    }
    bool is_sending_in_progress = false;
    {
      // Protect the queue from concurrent access
      std::lock_guard<std::mutex> lock(send_mutex_);
      is_sending_in_progress = !msgs_to_send_.empty();
      msgs_to_send_.emplace_back(std::move(msg));
    }
    if (!is_sending_in_progress)
    {
      send_message_from_queue();
    }
  }

  bool is_active() const
  {
    return socket_.is_open();
  }

private:
  void wait_for_incoming_message()
  {
    auto self(shared_from_this());
    // Asynchronously wait for a new message and read its header once received:
    boost::asio::async_read(socket_,
      boost::asio::buffer(read_msg_.data(), Message::HEADER_SIZE),
      [this, self](boost::system::error_code ec, std::size_t /*length*/)
      {
        if (ec)
        {
          std::cout << "Error while reading message header: " << ec.message()
                    << std::endl;
          socket_.close();
          return;
        }

        auto now = high_resolution_clock::now();
        if (read_msg_.decode_header())
        {
          if (read_msg_.is_text())
          {
            // Text message received, proceed to reading of message body
            read_message_body();
            return;
          }
          // Ack received, find the corresponding message ID in the table of timestamps
          auto iter = msg_send_times_.find(read_msg_.id());
          if (iter != msg_send_times_.end())
          {
            double elapsed_ms =
                std::chrono::duration_cast<std::chrono::microseconds>
                  (now - iter->second).count();
            std::cout << "    (message #" << read_msg_.id() << " delivered in "
                      << elapsed_ms/1000 << " [ms])" << std::endl;
            // We no longer need this entry, free the space:
            msg_send_times_.erase(iter);
          }
        }
        else
        {
          std::cout << "<Malformed message received>" << std::endl;
        }
        wait_for_incoming_message();
      });
  }

  void read_message_body()
  {
    auto self(shared_from_this());
    boost::asio::async_read(socket_,
      boost::asio::buffer(read_msg_.body(), read_msg_.body_size()),
      [this, self](boost::system::error_code ec, std::size_t /*length*/)
      {
        if (ec)
        {
          std::cout << "Error while reading message body: " << ec.message()
                  << std::endl;
          socket_.close();
          return;
        }

        if (read_msg_.is_text())
        {
          std::cout << ">>> ";
          std::cout.write(read_msg_.body(), read_msg_.body_size());
          std::cout << std::endl;

          // Send delivery acknowledgement to the sender
          Message ack(Message::Type::ack, read_msg_.id());
          ack.encode_header();
          send(std::move(ack));
        }

        // Finished processing, wait for another message:
        wait_for_incoming_message();
      });
  }

  void send_message_from_queue()
  {
    // Protect from concurrent access to the queue:
    std::lock_guard<std::mutex> lock(send_mutex_);
    if (msgs_to_send_.empty())
    {
      return;
    }
    auto self(shared_from_this());
    // Asynchronously send the first message from the queue:
    boost::asio::async_write(socket_,
      boost::asio::buffer(msgs_to_send_.front().data(), msgs_to_send_.front().size()),
      [this, self](boost::system::error_code ec, std::size_t /*length*/)
      {
        if (ec)
        {
          std::cout << "Error while sending: " << ec.message() << std::endl;
          socket_.close();
          return;
        }

        {
          // Protect from multi-threaded access to msgs_to_send_
          std::lock_guard<std::mutex> lock(send_mutex_);
          // Save the timestamp of sent message to 
          if (msgs_to_send_.front().is_text())
          {
            msg_send_times_.insert({ msgs_to_send_.front().id(),
                                     high_resolution_clock::now() });
          }
          msgs_to_send_.pop_front();
        }
        // In case there are more messages in the queue, send them as well:
        send_message_from_queue();
      });
  }

  tcp::socket socket_;
  Message read_msg_;
  std::deque<Message> msgs_to_send_;
  std::mutex send_mutex_;
  std::unordered_map<uint32_t, high_resolution_clock::time_point> msg_send_times_;
};
