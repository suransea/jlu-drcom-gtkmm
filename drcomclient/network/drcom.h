//
// Created by sea on 12/24/18.
//

#ifndef JLU_DRCOM_DRCOM_H
#define JLU_DRCOM_DRCOM_H

#include <memory>
#include <random>
#include <string>

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <sigc++/sigc++.h>
#include <spdlog/spdlog.h>

namespace DrcomClient {

typedef unsigned char byte;

typedef sigc::signal<void, bool, const std::string &> DrcomSignal;

class Drcom {
public:
  Drcom();

  Drcom(const Drcom &) = delete;

  ~Drcom();

  void do_login(const std::string &user, const std::string &password);

  void do_logout();

  void cancel();

  DrcomSignal &signal_login();

  DrcomSignal &signal_logout();

  DrcomSignal &signal_abort();

protected:
  bool init_socket();

  void login();

  void alive(int type);

  void challenge(bool logout);

  void logout();

  void on_recv_by_challenge(const boost::system::error_code &error,
                            std::size_t len, bool logout);

  void on_recv_by_login(const boost::system::error_code &error,
                        std::size_t len);

  void on_recv_by_alive(const boost::system::error_code &error, std::size_t len,
                        int type);

  void on_recv_by_logout(const boost::system::error_code &error,
                         std::size_t len);

  std::vector<byte> make_login_packet();

  std::vector<byte> make_alive_packet(int type);

  std::vector<byte> make_challenge_packet();

  std::vector<byte> make_logout_packet();

  template <typename Handler> void async_recv(Handler &&handler);

  DrcomSignal signal_login_{};
  DrcomSignal signal_logout_{};
  DrcomSignal signal_abort_{};

  bool login_status_{false};
  int retry_times_{-1};
  int keep38_count_{-1};
  int keep40_count_{-1};

  std::shared_ptr<spdlog::logger> logger_{spdlog::get("drcom")};
  std::random_device random_{};
  boost::asio::io_context io_context_{};
  boost::asio::io_context::work work_{io_context_};
  boost::asio::ip::udp::socket socket_{io_context_};
  boost::asio::ip::udp::endpoint remote_endpoint_{};
  boost::array<byte, 1024> recv_buffer_{0};
  sigc::connection time_out_{};

  std::array<byte, 4> host_ip_{0};
  std::array<byte, 16> md5a_{0};
  std::array<byte, 4> salt_{0};
  std::array<byte, 16> tail_{0};
  std::array<byte, 4> flux_{0};
  std::array<byte, 2> alive_ver_{0};

  std::string user_{};
  std::string password_{};
};

} // namespace DrcomClient

#endif // JLU_DRCOM_DRCOM_H