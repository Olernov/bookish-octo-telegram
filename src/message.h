#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

// This class provides functionality for binary representation of messages
// sent between chat participants.
// Binary message has the following format:
// [HEADER]
//   [type]       1 byte         0 - text/data, 1 - delivery acknowledgement
//   [msg_id]     4 bytes        sequential ID of the message
//   [body_size]  2 bytes        size of message body in bytes: minimum 1 byte,
//                               maximum 1024. 0 for acknowledgements
// [BODY]                        only present for text/data messages
//   [text/data of size from 1 to 1024]
class Message
{
public:
  static const size_t HEADER_SIZE = 7;
  static const size_t MAX_BODY_SIZE = 1024;

  enum class Type
  {
    text, // text message
    ack   // delivery acknowledgement from the remote side
  };

  Message() : msg_type_(Type::text), msg_id_(0) {}

  Message(Type msg_type, uint32_t msg_id, std::size_t msg_size = 0)
    : msg_type_(msg_type), msg_id_(msg_id), body_size_(msg_size)
  {
  }

  const char* data() const
  {
    return data_;
  }

  char* data()
  {
    return data_;
  }

  std::size_t size() const
  {
    return HEADER_SIZE + body_size_;
  }

  const char* body() const
  {
    return data_ + HEADER_SIZE;
  }

  char* body()
  {
    return data_ + HEADER_SIZE;
  }

  std::size_t body_size() const
  {
    return body_size_;
  }

  uint32_t id() const
  {
    return msg_id_;
  }

  bool is_text() const
  {
    return msg_type_ == Type::text;
  }

  bool is_ack() const
  {
    return msg_type_ == Type::ack;
  }

  void set_body_size(std::size_t new_size)
  {
    body_size_ = new_size;
    if (body_size_ > MAX_BODY_SIZE)
      body_size_ = MAX_BODY_SIZE;
  }

  bool decode_header()
  {
    switch(data_[0])
    {
      case 0:
        msg_type_ = Type::text;
        break;
      case 1:
        msg_type_ = Type::ack;
        break;
      default:
        return false;
    }

    // Convert values from network to host byte order:
    msg_id_ = ntohl(*(uint32_t*)(data_ + 1));
    body_size_ = ntohs(*(uint16_t*)(data_ + 5));
    if (body_size_ > MAX_BODY_SIZE)
    {
      body_size_ = 0;
      return false;
    }
    return true;
  }

  void encode_header()
  {
    data_[0] = (msg_type_ == Type::text ? 0 : 1);
    // Convert values from host to network byte order:
    *(uint32_t*)(data_ + 1) = htonl(msg_id_);
    *(uint16_t*)(data_+ 5) = htons(body_size_);
  }

private:
  Type msg_type_;
  uint32_t msg_id_;
  char data_[HEADER_SIZE + MAX_BODY_SIZE];
  std::size_t body_size_;
};
