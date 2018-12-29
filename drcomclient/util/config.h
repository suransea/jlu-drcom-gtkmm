//
// Created by sea on 12/23/18.
//

#ifndef JLU_DRCOM_CONFIG_H
#define JLU_DRCOM_CONFIG_H

#include <string>

namespace drcomclient {

const unsigned short PORT = 61440;

class Config {
public:
  Config();

  Config(const Config &) = delete;

  void save();

  // getters and setters
  const std::string &user() const;

  const std::string &password() const;

  const std::string &server_ip() const;

  const std::string &mac_address() const;

  bool auto_min() const;

  bool remember_me() const;

  bool auto_login() const;

public:
  void set_user(const std::string &user);

  void set_password(const std::string &password);

  void set_server_ip(const std::string &server_ip);

  void set_mac_address(const std::string &mac_address);

  void set_auto_min(bool auto_min);

  void set_remember_me(bool remember_me);

  void set_auto_login(bool auto_login);

private:
  std::string user_;
  std::string password_;
  std::string server_ip_;
  std::string mac_address_;
  bool auto_min_;
  bool remember_me_;
  bool auto_login_;
};

} //namespace drcomclient

#endif //JLU_DRCOM_CONFIG_H
