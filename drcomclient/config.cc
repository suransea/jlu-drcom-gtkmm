//
// Created by sea on 12/23/18.
//

#include "config.h"

#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

#include "drcomclient/network/mac_address.h"

namespace DrcomClient {

namespace {

std::string config_path() {
  return getenv("HOME") + std::string("/.config/drcom-client.conf");
}

constexpr const char *DEFAULT_SERVER_IP = "10.100.61.3";
constexpr const char *DEFAULT_MAC_ADDRESS = "00:00:00:00:00:00@config";
constexpr const char *KEY_USER = "user";
constexpr const char *KEY_PASSWD = "passwd";
constexpr const char *KEY_REMEMBER_ME = "remember_me";
constexpr const char *KEY_AUTO_LOGIN = "auto_login";
constexpr const char *KEY_AUTO_MIN = "auto_min";
constexpr const char *KEY_SERVER_IP = "server_ip";
constexpr const char *KEY_MAC_ADDRESS = "mac_address";

} // namespace

Config::Config()
    : server_ip_(DEFAULT_SERVER_IP), mac_address_(DEFAULT_MAC_ADDRESS) {
  std::ifstream fin(config_path());
  if (!fin) {
    auto &&address = MacAddress::all();
    if (!address.empty())
      mac_address_ = address[0].to_string();
    save();
    return;
  }
  std::string json_str((std::istreambuf_iterator<char>(fin)),
                       std::istreambuf_iterator<char>());
  fin.close();
  auto &&json = nlohmann::json::parse(json_str);
  user_ = json[KEY_USER];
  password_ = json[KEY_PASSWD];
  remember_me_ = json[KEY_REMEMBER_ME];
  auto_login_ = json[KEY_AUTO_LOGIN];
  auto_min_ = json[KEY_AUTO_MIN];
  server_ip_ = json[KEY_SERVER_IP];
  mac_address_ = json[KEY_MAC_ADDRESS];
}

void Config::save() {
  nlohmann::json json = {{KEY_USER, user_},
                         {KEY_PASSWD, password_},
                         {KEY_REMEMBER_ME, remember_me_},
                         {KEY_AUTO_LOGIN, auto_login_},
                         {KEY_AUTO_MIN, auto_min_},
                         {KEY_SERVER_IP, server_ip_},
                         {KEY_MAC_ADDRESS, mac_address_}};
  std::ofstream fout(config_path());
  fout << json << std::endl;
  fout.close();
}

const std::string &Config::user() const { return user_; }

const std::string &Config::password() const { return password_; }

const std::string &Config::server_ip() const { return server_ip_; }

const std::string &Config::mac_address() const { return mac_address_; }

bool Config::auto_min() const { return auto_min_; }

bool Config::remember_me() const { return remember_me_; }

bool Config::auto_login() const { return auto_login_; }

void Config::set_user(const std::string &user) { user_ = user; }

void Config::set_password(const std::string &password) { password_ = password; }

void Config::set_server_ip(const std::string &server_ip) {
  server_ip_ = server_ip;
}

void Config::set_mac_address(const std::string &mac_address) {
  mac_address_ = mac_address;
}

void Config::set_auto_min(bool auto_min) { auto_min_ = auto_min; }

void Config::set_remember_me(bool remember_me) { remember_me_ = remember_me; }

void Config::set_auto_login(bool auto_login) { auto_login_ = auto_login; }

} // namespace DrcomClient