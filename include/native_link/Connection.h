#pragma once

#include <string>
#include <cstdint>
#include <cmath>
#include <ostream>
#include <vector>

#include "Packet.hpp"

class ConnectionImpl;

class Connection
{
public:
  class Statistics
  {
  public:
    Statistics()
    {
      reset();
    }
    
    void reset()
    {
      sent_count = 0;
      receive_count = 0;
      enqueued_count = 0;
      ack_count = 0;
      rssi_latest = 0;
    }

    friend std::ostream &operator<<(std::ostream &out, const Connection::Statistics &s)
    {
      out << "Statistics(";
      out << "sent_count=" << s.sent_count;
      out << ",receive_count=" << s.receive_count;
      out << ",enqueued_count=" << s.enqueued_count;
      out << ",ack_count=" << s.ack_count;
      out << ",rssi_latest=" << (int)s.rssi_latest;
      out << ")";

      return out;
    }

    size_t sent_count;
    size_t receive_count;
    size_t enqueued_count;
    size_t ack_count;
    uint8_t rssi_latest;
  };

  class Settings
  {
    public:
      Settings()
        : use_safelink(true)
      {

      }

      bool use_safelink;
  };

public:

  // non construction-copyable
  Connection(const Connection&) = delete;

  // non copyable
  Connection& operator=(const Connection&) = delete;

  // construct, given a URI, e.g. "radio://*/80/2M/*"
  Connection(const std::string& uri, const Connection::Settings& settings = Connection::Settings());

  // deconstruct/cleanup
  ~Connection();

  static std::vector<std::string> scan(const std::string& address);

  void send(const Packet& p);

  Packet recv(unsigned int timeout_in_ms);

  const std::string& uri() const;

  Connection::Statistics statistics();

  friend std::ostream& operator<<(std::ostream& out, const Connection& p);

private:
  std::shared_ptr<ConnectionImpl> impl_;
};
