//
// Created by sea on 12/24/18.
//

#ifndef JLU_DRCOM_DRCOM_H
#define JLU_DRCOM_DRCOM_H

#include <string>
#include <random>
#include <thread>
#include <memory>

#include <boost/asio.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <sigc++/sigc++.h>

#include "spdlog/sinks/basic_file_sink.h"
#include "../util/bytes.h"

namespace drcomclient {

typedef sigc::signal<void, bool, const std::string &> DrcomSignal;

class Drcom {
public:
  Drcom() = default;

  Drcom(const Drcom &) = delete;

  void do_login(const std::string &user, const std::string &password);

  void do_logout();

  void cancel();

  DrcomSignal &signal_login();

  DrcomSignal &signal_logout();

  DrcomSignal &signal_abort();

protected:
  void init_socket();

  void login();

  void alive();

  void challenge(int times);

  void on_recv_by_challenge(const boost::system::error_code &error, std::size_t len);

  void on_recv_by_login(const boost::system::error_code &error, std::size_t len);

  void on_recv_by_alive(const boost::system::error_code &error, std::size_t len, int type);

  void on_recv_by_logout(const boost::system::error_code &error, std::size_t len);

  size_t make_login_packet(byte *&data);

  size_t make_alive_packet(int type, int rand, byte *&data);

  size_t make_challenge_packet(int times, byte *&data);

  size_t make_logout_packet(byte *&data);

  void emit_signal_safely(const DrcomSignal &signal, bool, const std::string &);

  DrcomSignal signal_login_;

  DrcomSignal signal_logout_;

  DrcomSignal signal_abort_;

  bool login_status_ = false;
  bool socket_inited_ = false;

  std::shared_ptr<spdlog::logger> logger_{spdlog::get("drcom")};
  std::random_device random_;
  boost::asio::io_context io_context_;
  boost::asio::ip::udp::socket socket_{io_context_};
  boost::asio::ip::udp::endpoint remote_endpoint_;
  boost::array<byte, 1024> recv_buffer_{0x00};
  byte host_ip_[4]{0x00};
  byte md5a_[16]{0x00};
  byte salt_[4]{0x00};
  byte tail_[16]{0x00};
  byte flux_[4]{0x00};
  std::string user;
  std::string password;
};

} //namespace drcomclient


#endif //JLU_DRCOM_DRCOM_H
