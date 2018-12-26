//
// Created by sea on 12/24/18.
//

#include "drcom.h"

#include <thread>
#include <exception>

#include <openssl/md5.h>
#include <glibmm.h>

#include "singleton.h"
#include "mac_address.h"
#include "../util/config.h"

namespace drcomclient {

using namespace boost;
using namespace boost::asio;
using boost::asio::ip::udp;

namespace {

byte *checksum(const byte *data, const size_t len) {
  byte *sum = new byte[4];
  byte *ret = new byte[4];
  sum[0] = 0x00;
  sum[1] = 0x00;
  sum[2] = 0x04;
  sum[3] = (byte) 0xd2;
  int i = 0;
  while (i + 3 < len) {
    sum[0] ^= data[i + 3];
    sum[1] ^= data[i + 2];
    sum[2] ^= data[i + 1];
    sum[3] ^= data[i];
    i += 4;
  }
  if (i < len) {
    byte *tmp = new byte[4]{0};
    for (int j = 3; j >= 0 && i < len; --j) {
      tmp[j] = data[i++];
    }
    for (int j = 0; j < 4; j++) {
      sum[j] = sum[j] ^ tmp[j];
    }
    delete[] tmp;
  }
  unsigned long num = 0;
  num |= sum[0];
  num <<= 8;
  num |= sum[1];
  num <<= 8;
  num |= sum[2];
  num <<= 8;
  num |= sum[3];
  num *= 1968;
  delete[] sum;
  for (int j = 0; j < 4; ++j) {
    ret[j] = (byte) (num & 0xff);
    num >>= 8;
  }
  return ret;
}

int challenge_times = 0;

} //namespace

void Drcom::init_socket() {
  if (socket_inited_)return;
  try {
    if (!socket_.is_open())socket_.open(udp::v4());
    socket_.bind(udp::endpoint(udp::v4(), PORT));
    socket_inited_ = true;
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
  }
}


void Drcom::do_login(const std::string &user, const std::string &password) {
  init_socket();
  if (!socket_inited_) return;
  this->user = user;
  this->password = password;
  auto config = Singleton<Config>::instance();
  auto &&address = ip::address::from_string(config->server_ip());
  auto port = PORT;
  remote_endpoint_ = ip::udp::endpoint(address, port);
  challenge(challenge_times++);
}

void Drcom::do_logout() {
  byte *packet;
  size_t len = make_logout_packet(packet);
  auto &&buf = buffer(packet, len);
  try {
    socket_.send_to(buf, remote_endpoint_);
    socket_.async_receive_from(
        buffer(recv_buffer_), remote_endpoint_,
        bind(&Drcom::on_recv_by_logout, this,
             asio::placeholders::error,
             asio::placeholders::bytes_transferred));
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
    return;
  }
  std::thread([&] { io_context_.run(); }).detach();
  Glib::signal_timeout().connect_once([&] {
    if (login_status_) {
      cancel();
      emit_signal_safely(signal_logout_, true, "Success");
    }
  }, 500);
  delete[] packet;
}

void Drcom::login() {
  byte *packet;
  size_t len = make_login_packet(packet);
  auto &&buf = buffer(packet, len);
  try {
    socket_.send_to(buf, remote_endpoint_);
    socket_.async_receive_from(
        buffer(recv_buffer_), remote_endpoint_,
        bind(&Drcom::on_recv_by_login, this,
             asio::placeholders::error,
             asio::placeholders::bytes_transferred));
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
    return;
  }
  std::thread([&] { io_context_.run(); }).detach();
  delete[] packet;
}

void Drcom::alive() {
  int rand = random_() % 0xffff;
  for (int i = 0; i < 3; i++) {
    byte *packet;
    size_t len = make_alive_packet(i, rand, packet);
    auto &&buf = buffer(packet, len);
    try {
      socket_.send_to(buf, remote_endpoint_);
      socket_.async_receive_from(
          buffer(recv_buffer_), remote_endpoint_,
          bind(&Drcom::on_recv_by_alive, this,
               asio::placeholders::error,
               asio::placeholders::bytes_transferred, i));
    } catch (std::exception &ex) {
      emit_signal_safely(signal_login_, false, ex.what());
      logger_->error(ex.what());
      cancel();
      return;
    }
    std::thread([&] { io_context_.run(); }).detach();
    delete[] packet;
  }
}

void Drcom::challenge(const int times) {
  byte *packet;
  size_t len = make_challenge_packet(times, packet);
  auto &&buf = buffer(packet, len);
  try {
    socket_.send_to(buf, remote_endpoint_);
    socket_.async_receive_from(
        buffer(recv_buffer_), remote_endpoint_,
        bind(&Drcom::on_recv_by_challenge, this,
             asio::placeholders::error,
             asio::placeholders::bytes_transferred));
    std::thread([&] { io_context_.run(); }).detach();
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
  }
  delete[] packet;
}

size_t Drcom::make_alive_packet(const int type, const int rand, byte *&data) {
  logger_->info("[alive{}] making packet", type);
  size_t data_len = 0;
  if (type == 0) {
    data_len = 38;
    data = new byte[data_len]{0x00};
    data[0] = 0xff;
    memcpy(data + 1, md5a_, 16);
    memcpy(data + 20, tail_, 16);
    data[36] = (byte) (rand >> 8);
    data[37] = (byte) (rand & 0xffffffff);
  } else {
    data_len = 40;
    data = new byte[data_len]{0x00};
    data[0] = 0x07;
    data[1] = 0x00;
    data[2] = 0x28;
    data[4] = 0x0b;
    data[5] = (byte) (2 * type - 1);
    data[6] = 0xdc;
    data[7] = 0x02;
    data[9] = (byte) (rand >> 8);
    data[10] = (byte) rand;
    memcpy(data + 16, flux_, 4);
    if (type == 2) {
      memcpy(data + 28, host_ip_, 4);
    }
  }
  logger_->debug("[alive{}] {}", type, bytes_to_string(data, data_len));
  return data_len;
}

size_t Drcom::make_challenge_packet(const int times, byte *&data) {
  logger_->info("[challenge] making packet");
  size_t data_len = 20;
  data = new byte[data_len]{0x00};
  data[0] = 0x01;
  data[1] = (byte) (0x02 + times);
  data[2] = (byte) (random() % 255);
  data[3] = (byte) (random() % 255);
  data[4] = 0x6a;
  logger_->debug("[challenge] {}", bytes_to_string(data, data_len));
  return data_len;
}

size_t Drcom::make_login_packet(byte *&data) {
  logger_->info("[login] making package");
  auto config = Singleton<Config>::instance();
  MacAddress mac(config->mac_address());
  byte code = 0x03;
  byte type = 0x01;
  byte eof = 0x00;
  byte control_check = 0x20;
  byte adapter_num = 0x05;
  byte ip_dog = 0x01;
  size_t salt_len = 4;
  byte primary_dns[4]{10};
  byte dhcp[4]{0x00};
  byte md5_sum[16]{0x00};
  byte md5_src[128];
  size_t md5_len;

  size_t passwd_len = password.length();
  if (passwd_len > 16)passwd_len = 16;
  size_t data_len = 334 + (passwd_len - 1) / 4 * 4;
  data = new byte[data_len]{0x00};

  //begin
  data[0] = code;
  data[1] = type;
  data[2] = eof;
  data[3] = (byte) (user.length() + 20);

  md5_len = 2 + salt_len + passwd_len;
  memset(md5_src, 0x00, md5_len);
  md5_src[0] = code;
  md5_src[1] = type;
  memcpy(md5_src + 2, salt_, salt_len);
  memcpy(md5_src + 2 + salt_len, password.c_str(), passwd_len);
  MD5(md5_src, md5_len, md5_sum);

  //md5a
  memcpy(md5a_, md5_sum, 16);
  memcpy(data + 4, md5_sum, 16);

  //username
  memcpy(data + 20, user.c_str(), user.length());

  data[56] = control_check;
  data[57] = adapter_num;

  //md5a[0:6] xor mac
  memcpy(data + 58, md5a_, 6);
  for (int i = 0; i < 6; ++i) {
    data[58 + i] = ((byte) data[58 + i]) ^ ((byte) mac.mac()[i]);
  }

  md5_len = 1 + passwd_len + salt_len + 4;
  memset(md5_src, 0x00, md5_len);
  md5_src[0] = 0x01;
  memcpy(md5_src + 1, password.c_str(), passwd_len);
  memcpy(md5_src + 1 + passwd_len, salt_, salt_len);
  MD5(md5_src, md5_len, md5_sum);

  //md5b
  memcpy(data + 64, md5_sum, 16);

  //ip address
  data[80] = 0x01;
  memcpy(data + 81, host_ip_, 4);

  //md5c
  data[97] = 0x14;//these are temp, 97:105 are md5c[0:8]
  data[98] = 0x00;
  data[99] = 0x07;
  data[100] = 0x0b;
  md5_len = 101;
  memcpy(md5_src, data, 101);
  MD5(md5_src, md5_len, md5_sum);
  memcpy(data + 97, md5_sum, 8);

  data[105] = ip_dog;
  auto &&host_name = ip::host_name();
  memcpy(data + 110, host_name.c_str(), host_name.length());
  memcpy(data + 142, primary_dns, 4);
  memcpy(data + 146, dhcp, 4);

  //second dns: 4, delimiter: 8
  data[162] = (byte) 0x94;
  data[166] = 0x06;
  data[170] = 0x02;
  data[174] = (byte) 0xf0;
  data[175] = 0x23;
  data[178] = 0x02;

  //drcom check
  data[182] = 0x44;
  data[183] = 0x72;
  data[184] = 0x43;
  data[185] = 0x4f;
  data[186] = 0x4d;
  data[187] = 0x00;
  data[188] = (byte) 0xcf;
  data[189] = 0x07;
  data[190] = 0x6a;
  memcpy(data + 246, "1c210c99585fd22ad03d35c956911aeec1eb449b", 40);
  data[310] = 0x6a;
  data[313] = (byte) passwd_len;
  byte *ror = new byte[passwd_len]{0x00};
  for (int i = 0, x; i < passwd_len; i++) {
    x = (((byte) md5a_[i]) & 0xff) ^ (((byte) password[i]) & 0xff);
    ror[i] = (byte) ((x << 3) + (x >> 5));
  }
  memcpy(data + 314, ror, passwd_len);
  delete[] ror;
  data[314 + passwd_len] = 0x02;
  data[315 + passwd_len] = 0x0c;

  //checksum(data+'\x01\x26\x07\x11\x00\x00'+dump(mac))
  data[316 + passwd_len] = 0x01;
  data[317 + passwd_len] = 0x26;
  data[318 + passwd_len] = 0x07;
  data[319 + passwd_len] = 0x11;
  data[320 + passwd_len] = 0x00;
  data[321 + passwd_len] = 0x00;
  memcpy(data + 322 + passwd_len, mac.mac(), 4);
  size_t tmp_len = 326 + passwd_len;
  byte *tmp = new byte[tmp_len]{0x00};
  memcpy(tmp, data, tmp_len);
  byte *sum = checksum(tmp, tmp_len);
  delete[] tmp;
  memcpy(data + 316 + passwd_len, sum, 4);
  delete[] sum;

  data[320 + passwd_len] = 0x00;
  data[321 + passwd_len] = 0x00;

  memcpy(data + 322 + passwd_len, mac.mac(), 6);
  size_t zero = (4 - passwd_len % 4) % 4;

  data[328 + passwd_len + zero] = (byte) (random_() % 255);
  data[329 + passwd_len + zero] = (byte) (random_() % 255);

  logger_->debug("[login] {}", bytes_to_string(data, data_len));
  return data_len;
}

size_t Drcom::make_logout_packet(byte *&data) {
  logger_->info("[logout] making logout packet");
  size_t data_len = 80;
  data = new byte[data_len]{0x00};
  data[0] = 0x06;
  data[1] = 0x01;
  data[2] = 0x00;
  data[3] = (byte) (user.length() + 20);
  size_t md5_len = 2 + 4 + password.length();
  byte *md5_src = new byte[md5_len]{0x00};
  memcpy(md5_src, data, 2);
  memcpy(md5_src + 2, salt_, 4);
  memcpy(md5_src + 6, password.c_str(), password.length());
  byte md5_sum[16]{0x00};
  MD5(md5_src, md5_len, md5_sum);
  delete[] md5_src;
  memcpy(data + 4, md5_sum, 16);
  memcpy(data + 20, user.c_str(), user.length() > 36 ? 36 : user.length());
  data[56] = 0x20;
  data[57] = 0x05;
  auto config = Singleton<Config>::instance();
  auto &&mac_str = config->mac_address();
  MacAddress mac(mac_str);
  for (int i = 0; i < 6; ++i) {
    data[58 + i] = data[4 + i] ^ mac.mac()[i];
  }
  memcpy(data + 64, tail_, 16);
  return data_len;
}

void Drcom::on_recv_by_challenge(const boost::system::error_code &error, std::size_t len) {
  if (error) {
    logger_->error(error.message());
    emit_signal_safely(signal_login_, false, error.message());
    cancel();
    return;
  }
  logger_->debug("[recv] {}", bytes_to_string(recv_buffer_.data(), len));
  if (recv_buffer_[0] != 0x02) {
    std::string msg = "[challenge] Challenge failed, unrecognized response: ";
    msg += std::to_string(recv_buffer_[0]);
    logger_->error(msg);
    emit_signal_safely(signal_login_, false, msg);
    cancel();
    return;
  }
  memcpy(salt_, recv_buffer_.data() + 4, 4);
  logger_->info("[login] [salt] {}", bytes_to_string(salt_, 4));
  if (len >= 25) {
    memcpy(host_ip_, recv_buffer_.data() + 20, 4);
    logger_->info("[login] [ip] {}", bytes_to_string(host_ip_, 4));
  }
  login();
}

void Drcom::on_recv_by_login(const boost::system::error_code &error, std::size_t len) {
  if (error) {
    logger_->error(error.message());
    emit_signal_safely(signal_login_, false, error.message());
    cancel();
    return;
  }
  logger_->debug("[recv] {}", bytes_to_string(recv_buffer_.data(), len));
  if (recv_buffer_[0] != 0x04) {
    if (recv_buffer_[0] == 0x05) {
      if (recv_buffer_[4] == 0x0B) {
        std::string msg = "Invalid mac address";
        logger_->error(msg);
        emit_signal_safely(signal_login_, false, msg);
        cancel();
        return;
      }
      std::string msg = "Invalid username or password";
      logger_->error(msg);
      emit_signal_safely(signal_login_, false, msg);
      cancel();
      return;
    } else {
      std::string msg = "Unknown error";
      logger_->error(msg);
      emit_signal_safely(signal_login_, false, msg);
      cancel();
      return;
    }
  }
  memcpy(tail_, recv_buffer_.data() + 23, 16);
  login_status_ = true;
  std::string msg = "Login successfully";
  logger_->info(msg);
  emit_signal_safely(signal_login_, true, msg);
  alive();
  Glib::signal_timeout().connect([&]() -> bool {
    if (login_status_)alive();
    return login_status_;
  }, 20000);
}

void Drcom::on_recv_by_alive(const boost::system::error_code &error, std::size_t len, int type) {
  if (error) {
    logger_->error(error.message());
    emit_signal_safely(signal_abort_, false, error.message());
    cancel();
    return;
  }
  logger_->debug("[recv{}] {}", type, bytes_to_string(recv_buffer_.data(), len));
  if (recv_buffer_[0] != 0x07) {
    std::string msg = "[alive] Alive failed, unrecognized response: ";
    msg += std::to_string(recv_buffer_[0]);
    logger_->error(msg);
    emit_signal_safely(signal_abort_, false, msg);
    cancel();
    return;
  }
  if (type > 0) {
    memcpy(flux_, recv_buffer_.data() + 16, 4);
  }
}

void Drcom::on_recv_by_logout(const boost::system::error_code &error, std::size_t len) {
  if (error) {
    logger_->error(error.message());
    emit_signal_safely(signal_logout_, false, error.message());
    cancel();
    return;
  }
  logger_->debug("[recv] {}", bytes_to_string(recv_buffer_.data(), len));
  if (recv_buffer_[0] == 0x04) {
    emit_signal_safely(signal_logout_, true, "Success");
  } else {
    emit_signal_safely(signal_logout_, false, "Cannot logout right now, but alive has been stopped");
  }
  cancel();
}

void Drcom::cancel() {
  if (socket_.is_open()) socket_.close();
  io_context_.stop();
  socket_inited_ = false;
  login_status_ = false;
}

DrcomSignal &Drcom::signal_login() {
  return signal_login_;
}

DrcomSignal &Drcom::signal_logout() {
  return signal_logout_;
}

DrcomSignal &Drcom::signal_abort() {
  return signal_abort_;
}

void Drcom::emit_signal_safely(const DrcomSignal &signal, bool b, const std::string &s) {
  Glib::signal_idle().connect_once([=] {
    signal.emit(b, s);
  });
}


} //namespace drcomclient