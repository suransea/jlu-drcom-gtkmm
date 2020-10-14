//
// Created by sea on 12/24/18.
//

#include "drcom.h"

#include <iomanip>
#include <thread>

#include <glibmm.h>
#include <openssl/md5.h>

#include "drcomclient/config.h"
#include "mac_address.h"
#include "singleton.h"

namespace DrcomClient {

namespace asio = boost::asio;

namespace {

int challenge_times = -1;

void emit_signal_safely(const DrcomSignal &signal, bool success,
                        const std::string &msg) {
  Glib::signal_idle().connect_once(
      [&signal, success, msg] { signal.emit(success, msg); });
}

template <typename Data>
std::string bytes_to_string(const Data &data, std::size_t len) {
  std::stringstream ss{};
  ss << "[bytes]";
  for (int i = 0; i < len; ++i) {
    ss << " " << std::setiosflags(std::ios::uppercase) << std::hex
       << static_cast<int>(data[i]);
  }
  std::string str{};
  getline(ss, str);
  return str;
}

template <typename Data> std::string bytes_to_string(const Data &data) {
  return bytes_to_string(data, data.size());
}

std::array<byte, 4> checksum(const std::vector<byte> &data) {
  std::array<byte, 4> sum{};
  sum[0] = 0x00;
  sum[1] = 0x00;
  sum[2] = 0x04;
  sum[3] = (byte)0xd2;
  int i = 0;
  while (i + 3 < data.size()) {
    sum[0] ^= data[i + 3];
    sum[1] ^= data[i + 2];
    sum[2] ^= data[i + 1];
    sum[3] ^= data[i];
    i += 4;
  }
  if (i < data.size()) {
    std::array<byte, 4> tmp{0};
    for (int j = 3; j >= 0 && i < data.size(); --j) {
      tmp[j] = data[i++];
    }
    for (int j = 0; j < 4; j++) {
      sum[j] ^= tmp[j];
    }
  }
  unsigned long num = 0;
  num |= sum[0];
  num <<= 8u;
  num |= sum[1];
  num <<= 8u;
  num |= sum[2];
  num <<= 8u;
  num |= sum[3];
  num *= 1968;
  std::array<byte, 4> result{};
  for (int j = 0; j < 4; ++j) {
    result[j] = (byte)(num & (byte)0xff);
    num >>= 8u;
  }
  return result;
}

} // namespace

Drcom::Drcom() {
  std::thread([this] { io_context_.run(); }).detach();
}

Drcom::~Drcom() {
  cancel();
  io_context_.stop();
}

bool Drcom::init_socket() {
  try {
    if (!socket_.is_open()) {
      socket_.open(asio::ip::udp::v4());
      socket_.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), Config::port));
    }
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    if (socket_.is_open())
      socket_.close();
    return false;
  }
  return true;
}

void Drcom::do_login(const std::string &user, const std::string &password) {
  if (!init_socket())
    return;
  user_ = user;
  password_ = password;
  auto config = Singleton<Config>::instance();
  auto &&address = asio::ip::address::from_string(config->server_ip());
  remote_endpoint_ = std::move(asio::ip::udp::endpoint(address, Config::port));
  challenge(false);
}

void Drcom::do_logout() {
  if (!login_status_) {
    emit_signal_safely(signal_logout_, true, "[logout] Success");
    return;
  }
  challenge(true);
  logout();
}

void Drcom::login() {
  auto &&packet = make_login_packet();
  auto &&buf = asio::buffer(packet);
  try {
    socket_.send_to(buf, remote_endpoint_);
    auto &&handler =
        boost::bind(&Drcom::on_recv_by_login, this, asio::placeholders::error,
                    asio::placeholders::bytes_transferred);
    async_recv(handler);
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
  }
}

void Drcom::logout() {
  auto &&packet = make_logout_packet();
  auto &&buf = asio::buffer(packet);
  try {
    socket_.send_to(buf, remote_endpoint_);
    auto &&handler =
        bind(&Drcom::on_recv_by_logout, this, asio::placeholders::error,
             asio::placeholders::bytes_transferred);
    async_recv(handler);
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
  }
}

void Drcom::alive(int type) {
  if (type > 3)
    return;
  if (type == 2 && keep38_count_ % 10 != 0) {
    alive(3);
    return;
  }
  auto &&packet = make_alive_packet(type);
  auto &&buf = asio::buffer(packet);
  try {
    socket_.send_to(buf, remote_endpoint_);
    auto &&handler =
        bind(&Drcom::on_recv_by_alive, this, asio::placeholders::error,
             asio::placeholders::bytes_transferred, type);
    async_recv(handler);
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
  }
}

void Drcom::challenge(bool logout) {
  auto &&packet = make_challenge_packet();
  auto &&buf = asio::buffer(packet);
  try {
    socket_.send_to(buf, remote_endpoint_);
    auto &&handler =
        bind(&Drcom::on_recv_by_challenge, this, asio::placeholders::error,
             asio::placeholders::bytes_transferred, logout);
    async_recv(handler);
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
  }
}

std::vector<byte> Drcom::make_alive_packet(const int type) {
  logger_->info("[alive{}] making packet", type);
  std::vector<byte> data(0);
  if (type == 0) {
    data.resize(38, 0);
    data[0] = (byte)0xff;
    memcpy(&data[1], md5a_.data(), md5a_.size());
    memcpy(&data[20], tail_.data(), tail_.size());
    data[36] = (byte)(random_() % 255);
    data[37] = (byte)(random_() % 255);
    ++keep38_count_;
  } else {
    data.resize(40, 0);
    data[0] = 0x07;
    data[1] = (byte)++keep40_count_;
    data[2] = 0x28;
    data[4] = 0x0b;
    data[5] = 0x01;
    if (type == 3)
      data[5] = 0x03;
    memcpy(&data[6], alive_ver_.data(), alive_ver_.size());
    if (type == 1) {
      data[6] = 0x0f;
      data[7] = 0x27;
    }
    data[8] = 0x2f;
    data[9] = 0x12;
    memcpy(&data[16], flux_.data(), flux_.size());
    if (type == 3) {
      memcpy(&data[28], host_ip_.data(), host_ip_.size());
    }
  }
  logger_->debug("[alive{}] {}", type, bytes_to_string(data));
  return std::move(data);
}

std::vector<byte> Drcom::make_challenge_packet() {
  logger_->info("[challenge] making packet");
  std::vector<byte> data(20, 0);
  data[0] = 0x01;
  data[1] = (byte)(0x02 + (++challenge_times));
  data[2] = (byte)(random_() % 255);
  data[3] = (byte)(random_() % 255);
  data[4] = 0x6a;
  logger_->debug("[challenge] {}", bytes_to_string(data));
  return std::move(data);
}

std::vector<byte> Drcom::make_login_packet() {
  logger_->info("[login] making package");
  const byte code = 0x03;
  const byte type = 0x01;
  const byte eof = 0x00;
  const byte control_check = 0x20;
  const byte adapter_num = 0x05;
  const byte ip_dog = 0x01;
  const std::size_t salt_len = 4;

  std::array<byte, 4> primary_dns{10};
  std::array<byte, 4> dhcp{0};
  std::array<byte, 16> md5_sum{0};
  std::array<byte, 128> md5_src{0};

  auto config = Singleton<Config>::instance();
  MacAddress mac(config->mac_address());

  std::size_t passwd_len = password_.length();
  if (passwd_len > 16)
    passwd_len = 16;
  std::size_t data_len = 334 + (passwd_len - 1) / 4 * 4;
  std::vector<byte> data(data_len, 0);

  // begin
  data[0] = code;
  data[1] = type;
  data[2] = eof;
  data[3] = (byte)(user_.length() + 20);

  std::size_t md5_len = 2 + salt_len + passwd_len;
  md5_src[0] = code;
  md5_src[1] = type;
  memcpy(md5_src.data() + 2, salt_.data(), salt_len);
  memcpy(md5_src.data() + 2 + salt_len, password_.c_str(), passwd_len);
  MD5(md5_src.data(), md5_len, md5_sum.data());

  // md5a
  md5a_ = md5_sum;
  memcpy(&data[4], md5_sum.data(), md5_sum.size());

  // username
  memcpy(&data[20], user_.c_str(), user_.length());

  data[56] = control_check;
  data[57] = adapter_num;

  // md5a[0:6] xor mac
  memcpy(&data[58], md5a_.data(), 6);
  for (int i = 0; i < 6; ++i) {
    data[58 + i] = data[58 + i] ^ mac[i];
  }

  md5_len = 1 + passwd_len + salt_len + 4;
  md5_src[0] = 0x01;
  memcpy(md5_src.data() + 1, password_.c_str(), passwd_len);
  memcpy(md5_src.data() + 1 + passwd_len, salt_.data(), salt_len);
  MD5(md5_src.data(), md5_len, md5_sum.data());

  // md5b
  memcpy(&data[64], md5_sum.data(), md5_sum.size());

  // ip address
  data[80] = 0x01;
  memcpy(&data[81], host_ip_.data(), host_ip_.size());

  // md5c
  data[97] = 0x14; // these are temp, 97:105 are md5c[0:8]
  data[98] = 0x00;
  data[99] = 0x07;
  data[100] = 0x0b;
  md5_len = 101;
  memcpy(md5_src.data(), &data[0], 101);
  MD5(md5_src.data(), md5_len, md5_sum.data());
  memcpy(&data[97], md5_sum.data(), 8);

  data[105] = ip_dog;
  auto &&host_name = asio::ip::host_name();
  memcpy(&data[110], host_name.c_str(), host_name.length());
  memcpy(&data[142], primary_dns.data(), primary_dns.size());
  memcpy(&data[146], dhcp.data(), dhcp.size());

  // second dns: 4, delimiter: 8
  data[162] = (byte)0x94;
  data[166] = 0x06;
  data[170] = 0x02;
  data[174] = (byte)0xf0;
  data[175] = 0x23;
  data[178] = 0x02;

  // drcom check
  data[182] = 0x44;
  data[183] = 0x72;
  data[184] = 0x43;
  data[185] = 0x4f;
  data[186] = 0x4d;
  data[187] = 0x00;
  data[188] = (byte)0xcf;
  data[189] = 0x07;
  data[190] = 0x6a;
  memcpy(&data[246], "1c210c99585fd22ad03d35c956911aeec1eb449b", 40);
  data[310] = 0x6a;
  data[313] = (byte)passwd_len;
  std::vector<byte> ror(passwd_len, 0);
  for (int i = 0; i < passwd_len; i++) {
    unsigned x = (md5a_[i] & 0xffu) ^ (((byte)password_[i]) & 0xffu);
    ror[i] = (byte)((x << 3u) + (x >> 5u));
  }
  memcpy(&data[314], &ror[0], passwd_len);
  data[314 + passwd_len] = 0x02;
  data[315 + passwd_len] = 0x0c;

  // checksum(data+'\x01\x26\x07\x11\x00\x00'+dump(mac))
  data[316 + passwd_len] = 0x01;
  data[317 + passwd_len] = 0x26;
  data[318 + passwd_len] = 0x07;
  data[319 + passwd_len] = 0x11;
  data[320 + passwd_len] = 0x00;
  data[321 + passwd_len] = 0x00;
  memcpy(&data[322] + passwd_len, mac.mac().data(), 4);
  std::size_t tmp_len = 326 + passwd_len;
  std::vector<byte> tmp(tmp_len, 0);
  memcpy(&tmp[0], &data[0], tmp_len);
  auto &&sum = checksum(tmp);
  memcpy(&data[316] + passwd_len, &sum[0], 4);

  data[320 + passwd_len] = 0x00;
  data[321 + passwd_len] = 0x00;

  memcpy(&data[322] + passwd_len, mac.mac().data(), 6);
  std::size_t zero = (4 - passwd_len % 4) % 4;

  data[328 + passwd_len + zero] = (byte)(random_() % 255);
  data[329 + passwd_len + zero] = (byte)(random_() % 255);

  logger_->debug("[login] {}", bytes_to_string(data));
  return std::move(data);
}

std::vector<byte> Drcom::make_logout_packet() {
  logger_->info("[logout] making logout packet");
  std::vector<byte> data(80, 0);
  data[0] = 0x06;
  data[1] = 0x01;
  data[2] = 0x00;
  data[3] = (byte)(user_.length() + 20);
  std::size_t md5_len = 2 + 4 + password_.length();
  std::vector<byte> md5_src(md5_len, 0);
  memcpy(&md5_src[0], &data[0], 2);
  memcpy(&md5_src[2], salt_.data(), salt_.size());
  memcpy(&md5_src[6], password_.c_str(), password_.length());
  std::array<byte, 16> md5_sum{0};
  MD5(md5_src.data(), md5_len, md5_sum.data());
  memcpy(&data[4], md5_sum.data(), md5_sum.size());
  memcpy(&data[20], user_.c_str(), user_.length() > 36 ? 36 : user_.length());
  data[56] = 0x20;
  data[57] = 0x05;
  auto config = Singleton<Config>::instance();
  MacAddress mac(config->mac_address());
  for (int i = 0; i < 6; ++i) {
    data[58 + i] = data[4 + i] ^ mac[i];
  }
  memcpy(&data[64], tail_.data(), tail_.size());
  logger_->debug("[logout] {}", bytes_to_string(data));
  return std::move(data);
}

void Drcom::on_recv_by_challenge(const boost::system::error_code &error,
                                 std::size_t len, bool logout) {
  if (logout)
    return;
  if (error) {
    logger_->error(error.message());
    emit_signal_safely(signal_login_, false,
                       "[challenge] Operation cancel or timeout");
    cancel();
    return;
  }
  time_out_.disconnect();
  logger_->debug("[recv] {}", bytes_to_string(recv_buffer_, len));
  if (recv_buffer_[0] != 0x02) {
    std::string msg = "[challenge] Challenge failed, unrecognized response: ";
    msg += std::to_string(recv_buffer_[0]);
    logger_->error(msg);
    emit_signal_safely(signal_login_, false, msg);
    cancel();
    return;
  }
  memcpy(salt_.data(), recv_buffer_.data() + 4, salt_.size());
  logger_->info("[login] [salt] {}", bytes_to_string(salt_));
  if (len >= 25) {
    memcpy(host_ip_.data(), recv_buffer_.data() + 20, host_ip_.size());
    logger_->info("[login] [ip] {}", bytes_to_string(host_ip_));
  }
  login();
}

void Drcom::on_recv_by_login(const boost::system::error_code &error,
                             std::size_t len) {
  if (error) {
    logger_->error(error.message());
    emit_signal_safely(signal_login_, false,
                       "[login] Operation cancel or timeout");
    cancel();
    return;
  }
  logger_->debug("[recv] {}", bytes_to_string(recv_buffer_, len));
  time_out_.disconnect();
  if (recv_buffer_[0] != 0x04) {
    if (recv_buffer_[0] == 0x05) {
      if (recv_buffer_[4] == 0x0B) {
        std::string msg = "[login] Invalid mac address";
        logger_->error(msg);
        emit_signal_safely(signal_login_, false, msg);
        cancel();
        return;
      }
      std::string msg = "[login] Invalid username or password";
      logger_->error(msg);
      emit_signal_safely(signal_login_, false, msg);
      cancel();
      return;
    } else {
      std::string msg = "[login] Unknown error";
      logger_->error(msg);
      emit_signal_safely(signal_login_, false, msg);
      cancel();
      return;
    }
  }
  memcpy(tail_.data(), recv_buffer_.data() + 23, tail_.size());
  login_status_ = true;
  std::string msg = "[login] Login successfully";
  logger_->info(msg);
  emit_signal_safely(signal_login_, true, msg);
  keep38_count_ = -1;
  keep40_count_ = -1;
  alive(0);
}

void Drcom::on_recv_by_alive(const boost::system::error_code &error,
                             std::size_t len, int type) {
  if (error) {
    if (++retry_times_ > 10) {
      logger_->error(error.message());
      emit_signal_safely(signal_abort_, false, "[alive] timeout");
      cancel();
    } else {
      alive(0);
    }
    return;
  }
  logger_->debug("[recv{}] {}", type,
                 bytes_to_string(recv_buffer_.data(), len));
  time_out_.disconnect();
  retry_times_ = -1;
  if (recv_buffer_[0] != 0x07) {
    std::string msg = "[alive] Alive failed, unrecognized response: ";
    msg += std::to_string(recv_buffer_[0]);
    logger_->error(msg);
    emit_signal_safely(signal_abort_, false, msg);
    cancel();
    return;
  }
  if (type == 0) {
    memcpy(alive_ver_.data(), recv_buffer_.data() + 28, alive_ver_.size());
  } else if (type == 2) {
    memcpy(flux_.data(), recv_buffer_.data() + 16, flux_.size());
  } else if (type == 3) {
    Glib::signal_timeout().connect_once(
        [this] {
          if (login_status_)
            alive(0);
        },
        20000);
  }
  alive(type + 1);
}

void Drcom::on_recv_by_logout(const boost::system::error_code &error,
                              std::size_t len) {
  if (error) {
    logger_->error(error.message());
    emit_signal_safely(signal_logout_, true, "[logout] Success");
    cancel();
    return;
  }
  logger_->debug("[recv] {}", bytes_to_string(recv_buffer_.data(), len));
  time_out_.disconnect();
  if (recv_buffer_[0] == 0x04) {
    emit_signal_safely(signal_logout_, true, "[logout] Success");
  } else {
    emit_signal_safely(
        signal_logout_, false,
        "[logout] Cannot logout right now, but alive has been stopped");
  }
  cancel();
}

void Drcom::cancel() {
  if (socket_.is_open())
    socket_.close();
  time_out_.disconnect();
  login_status_ = false;
}

DrcomSignal &Drcom::signal_login() { return signal_login_; }

DrcomSignal &Drcom::signal_logout() { return signal_logout_; }

DrcomSignal &Drcom::signal_abort() { return signal_abort_; }

template <typename Handler> void Drcom::async_recv(Handler &&handler) {
  socket_.async_receive_from(asio::buffer(recv_buffer_), remote_endpoint_,
                             handler);
  time_out_ = Glib::signal_timeout().connect(
      [&]() -> bool {
        if (socket_.is_open())
          socket_.cancel();
        return false;
      },
      1000);
}

} // namespace DrcomClient