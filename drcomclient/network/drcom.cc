//
// Created by sea on 12/24/18.
//

#include "drcom.h"

#include <thread>

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

/**
 * 数据报的接收不在主线程中，不能安全触发含有更新UI操作的信号槽
 * 此方法将信号发送置于主线程的任务中
 * @param signal
 * @param bool
 * @param string
 */
void emit_signal_safely(const DrcomSignal &signal, bool b, const std::string &s) {
  Glib::signal_idle().connect_once([=] {
    signal.emit(b, s);
  });
}

/**
 * 由于IO任务是动态添加的，无法确定IO停止的时间
 * 每次添加io操作时调用此方法试图重启io context
 * @param io context
 */
void restart_io_context(io_context &io) {
  if (io.stopped()) {
    io.restart();
  }
}

int challenge_times = -1;

int keep_count = -1;

} //namespace

void Drcom::init_socket() {
  if (socket_inited_)return;
  try {
    if (!socket_.is_open()) {
      socket_.open(udp::v4());
      socket_.bind(udp::endpoint(udp::v4(), PORT));
      socket_inited_ = true;
    }
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    if (socket_.is_open()) socket_.close();
  }
}


void Drcom::do_login(const std::string &user, const std::string &password) {
  user_ = user;
  password_ = password;
  auto config = Singleton<Config>::instance();
  auto &&address = ip::address::from_string(config->server_ip());
  auto port = PORT;
  remote_endpoint_ = ip::udp::endpoint(address, port);
  init_socket();
  if (!socket_inited_) return;
  challenge(false);
}

void Drcom::do_logout() {
  if (!login_status_) {
    emit_signal_safely(signal_logout_, true, "[logout] Success");
    return;
  }
  challenge(true);
  byte *packet;
  std::size_t len = make_logout_packet(packet);
  auto &&buf = buffer(packet, len);
  restart_io_context(io_context_);
  try {
    socket_.send_to(buf, remote_endpoint_);
    socket_.async_receive_from(
        buffer(recv_buffer_), remote_endpoint_,
        bind(&Drcom::on_recv_by_logout, this,
             asio::placeholders::error,
             asio::placeholders::bytes_transferred));
    std::thread([&] { io_context_.run_one(); }).detach();
    Glib::signal_timeout().connect_once([&] {
      if (login_status_) {
        cancel();
        emit_signal_safely(signal_logout_, true, "[logout] Success");
      }
    }, 3000);
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
  }
  delete[] packet;
}

void Drcom::login() {
  byte *packet;
  std::size_t len = make_login_packet(packet);
  auto &&buf = buffer(packet, len);
  restart_io_context(io_context_);
  try {
    socket_.send_to(buf, remote_endpoint_);
    socket_.async_receive_from(
        buffer(recv_buffer_), remote_endpoint_,
        bind(&Drcom::on_recv_by_login, this,
             asio::placeholders::error,
             asio::placeholders::bytes_transferred));
    std::thread([&] { io_context_.run_one(); }).detach();
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
  }
  delete[] packet;
}

void Drcom::alive(int type) {
  io_context_.restart();
  if (type > 3) return;
  if (type == 2 && keep_count % 21 != 0) {
    alive(3);
    return;
  }
  byte *packet;
  std::size_t len = make_alive_packet(type, packet);
  auto &&buf = buffer(packet, len);
  restart_io_context(io_context_);
  try {
    socket_.send_to(buf, remote_endpoint_);
    socket_.async_receive_from(
        buffer(recv_buffer_), remote_endpoint_,
        bind(&Drcom::on_recv_by_alive, this,
             asio::placeholders::error,
             asio::placeholders::bytes_transferred, type));
    std::thread([&] { io_context_.run_one(); }).detach();
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
  }
  delete[] packet;
}

void Drcom::challenge(bool logout) {
  byte *packet;
  std::size_t len = make_challenge_packet(packet);
  auto &&buf = buffer(packet, len);
  restart_io_context(io_context_);
  try {
    socket_.send_to(buf, remote_endpoint_);
    socket_.async_receive_from(
        buffer(recv_buffer_), remote_endpoint_,
        bind(&Drcom::on_recv_by_challenge, this,
             asio::placeholders::error,
             asio::placeholders::bytes_transferred, logout));
    std::thread([&] { io_context_.run_one(); }).detach();
  } catch (std::exception &ex) {
    emit_signal_safely(signal_login_, false, ex.what());
    logger_->error(ex.what());
    cancel();
  }
  delete[] packet;
}

std::size_t Drcom::make_alive_packet(const int type, byte *&data) {
  logger_->info("[alive{}] making packet", type);
  std::size_t data_len = 0;
  if (type == 0) {
    data_len = 38;
    data = new byte[data_len]{0x00};
    data[0] = (byte) 0xff;
    memcpy(data + 1, md5a_, 16);
    memcpy(data + 20, tail_, 16);
    data[36] = (byte) (random_() % 255);
    data[37] = (byte) (random_() % 255);
  } else {
    data_len = 40;
    data = new byte[data_len]{0x00};
    data[0] = 0x07;
    data[1] = (byte) ++keep_count;
    data[2] = 0x28;
    data[4] = 0x0b;
    data[5] = 0x01;
    if (type == 3) data[5] = 0x03;
    data[6] = alive_ver[0];
    data[7] = alive_ver[1];
    if (type == 1) {
      data[6] = 0x0f;
      data[7] = 0x27;
    }
    data[8] = 0x2f;
    data[9] = 0x12;
    memcpy(data + 16, flux_, 4);
    if (type == 3) {
      memcpy(data + 28, host_ip_, 4);
    }
  }
  logger_->debug("[alive{}] {}", type, bytes_to_string(data, data_len));
  return data_len;
}

std::size_t Drcom::make_challenge_packet(byte *&data) {
  logger_->info("[challenge] making packet");
  std::size_t data_len = 20;
  data = new byte[data_len]{0x00};
  data[0] = 0x01;
  data[1] = (byte) (0x02 + (++challenge_times));
  data[2] = (byte) (random_() % 255);
  data[3] = (byte) (random_() % 255);
  data[4] = 0x6a;
  logger_->debug("[challenge] {}", bytes_to_string(data, data_len));
  return data_len;
}

std::size_t Drcom::make_login_packet(byte *&data) {
  logger_->info("[login] making package");
  auto config = Singleton<Config>::instance();
  MacAddress mac(config->mac_address());
  byte code = 0x03;
  byte type = 0x01;
  byte eof = 0x00;
  byte control_check = 0x20;
  byte adapter_num = 0x05;
  byte ip_dog = 0x01;
  std::size_t salt_len = 4;
  byte primary_dns[4]{10};
  byte dhcp[4]{0x00};
  byte md5_sum[16]{0x00};
  byte md5_src[128];
  std::size_t md5_len;

  std::size_t passwd_len = password_.length();
  if (passwd_len > 16)passwd_len = 16;
  std::size_t data_len = 334 + (passwd_len - 1) / 4 * 4;
  data = new byte[data_len]{0x00};

  //begin
  data[0] = code;
  data[1] = type;
  data[2] = eof;
  data[3] = (byte) (user_.length() + 20);

  md5_len = 2 + salt_len + passwd_len;
  memset(md5_src, 0x00, md5_len);
  md5_src[0] = code;
  md5_src[1] = type;
  memcpy(md5_src + 2, salt_, salt_len);
  memcpy(md5_src + 2 + salt_len, password_.c_str(), passwd_len);
  MD5(md5_src, md5_len, md5_sum);

  //md5a
  memcpy(md5a_, md5_sum, 16);
  memcpy(data + 4, md5_sum, 16);

  //username
  memcpy(data + 20, user_.c_str(), user_.length());

  data[56] = control_check;
  data[57] = adapter_num;

  //md5a[0:6] xor mac
  memcpy(data + 58, md5a_, 6);
  for (int i = 0; i < 6; ++i) {
    data[58 + i] = data[58 + i] ^ mac.mac()[i];
  }

  md5_len = 1 + passwd_len + salt_len + 4;
  memset(md5_src, 0x00, md5_len);
  md5_src[0] = 0x01;
  memcpy(md5_src + 1, password_.c_str(), passwd_len);
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
  auto ror = new byte[passwd_len]{0x00};
  for (int i = 0; i < passwd_len; i++) {
    unsigned x = (md5a_[i] & (byte) 0xff) ^(((byte) password_[i]) & (byte) 0xff);
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
  std::size_t tmp_len = 326 + passwd_len;
  auto tmp = new byte[tmp_len]{0x00};
  memcpy(tmp, data, tmp_len);
  auto sum = new byte[4];
  checksum(tmp, tmp_len, sum);
  delete[] tmp;
  memcpy(data + 316 + passwd_len, sum, 4);
  delete[] sum;

  data[320 + passwd_len] = 0x00;
  data[321 + passwd_len] = 0x00;

  memcpy(data + 322 + passwd_len, mac.mac(), 6);
  std::size_t zero = (4 - passwd_len % 4) % 4;

  data[328 + passwd_len + zero] = (byte) (random_() % 255);
  data[329 + passwd_len + zero] = (byte) (random_() % 255);

  logger_->debug("[login] {}", bytes_to_string(data, data_len));
  return data_len;
}

std::size_t Drcom::make_logout_packet(byte *&data) {
  logger_->info("[logout] making logout packet");
  std::size_t data_len = 80;
  data = new byte[data_len]{0x00};
  data[0] = 0x06;
  data[1] = 0x01;
  data[2] = 0x00;
  data[3] = (byte) (user_.length() + 20);
  std::size_t md5_len = 2 + 4 + password_.length();
  auto md5_src = new byte[md5_len]{0x00};
  memcpy(md5_src, data, 2);
  memcpy(md5_src + 2, salt_, 4);
  memcpy(md5_src + 6, password_.c_str(), password_.length());
  byte md5_sum[16]{0x00};
  MD5(md5_src, md5_len, md5_sum);
  delete[] md5_src;
  memcpy(data + 4, md5_sum, 16);
  memcpy(data + 20, user_.c_str(), user_.length() > 36 ? 36 : user_.length());
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

void Drcom::on_recv_by_challenge(const boost::system::error_code &error, std::size_t len, bool logout) {
  if (logout) return;
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
  memcpy(tail_, recv_buffer_.data() + 23, 16);
  login_status_ = true;
  std::string msg = "[login] Login successfully";
  logger_->info(msg);
  emit_signal_safely(signal_login_, true, msg);
  keep_count = -1;
  alive(0);
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
  if (type == 0) {
    alive_ver[0] = recv_buffer_[28];
    alive_ver[1] = recv_buffer_[29];
  } else if (type == 2) {
    memcpy(flux_, recv_buffer_.data() + 16, 4);
  } else if (type == 3) {
    Glib::signal_timeout().connect_once([&] {
      if (login_status_) alive(0);
    }, 20000);
  }
  alive(type + 1);
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
    emit_signal_safely(signal_logout_, true, "[logout] Success");
  } else {
    emit_signal_safely(signal_logout_, false, "[logout] Cannot logout right now, but alive has been stopped");
  }
  cancel();
}

void Drcom::cancel() {
  if (socket_.is_open()) socket_.close();
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


} //namespace drcomclient