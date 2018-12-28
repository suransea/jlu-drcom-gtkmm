//
// Created by sea on 12/23/18.
//

#include "config.h"

#include <fstream>
#include <iostream>

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "../network/mac_address.h"

namespace drcomclient {

namespace {

const std::string CONFIG_PATH() {
  return getenv("HOME") + std::string("/.config/drcom-client.conf");
}

const char *DEFAULT_SERVER_IP = "10.100.61.3";
const char *DEFAULT_MAC_ADDRESS = "00:00:00:00:00:00@config";
const char *KEY_USER = "user";
const char *KEY_PASSWD = "passwd";
const char *KEY_REMEMBER_ME = "remember_me";
const char *KEY_AUTO_LOGIN = "auto_login";
const char *KEY_AUTO_MIN = "auto_min";
const char *KEY_SERVER_IP = "server_ip";
const char *KEY_MAC_ADDRESS = "mac_address";


} //namespace

Config::Config() : server_ip_(DEFAULT_SERVER_IP),
                   mac_address_(DEFAULT_MAC_ADDRESS) {
  using namespace std;
  using namespace rapidjson;
  ifstream fin(CONFIG_PATH());
  if (!fin) {
    auto &&macs = MacAddress::get_macs();
    if (!macs.empty()) mac_address_ = macs.begin()->to_string();
    save();
    return;
  }
  string json_str((istreambuf_iterator<char>(fin)),
                  istreambuf_iterator<char>());
  fin.close();
  Document doc;
  doc.Parse(json_str.c_str());
  user_ = doc[KEY_USER].GetString();
  password_ = doc[KEY_PASSWD].GetString();
  remember_me_ = doc[KEY_REMEMBER_ME].GetBool();
  auto_login_ = doc[KEY_AUTO_LOGIN].GetBool();
  auto_min_ = doc[KEY_AUTO_MIN].GetBool();
  server_ip_ = doc[KEY_SERVER_IP].GetString();
  mac_address_ = doc[KEY_MAC_ADDRESS].GetString();
}

void Config::save() {
  using namespace std;
  using namespace rapidjson;
  Document doc;
  doc.SetObject();
  auto &&allocator = doc.GetAllocator();
  doc.AddMember(StringRef(KEY_USER), StringRef(user_.c_str()), allocator);
  doc.AddMember(StringRef(KEY_PASSWD), StringRef(password_.c_str()), allocator);
  doc.AddMember(StringRef(KEY_REMEMBER_ME), remember_me_, allocator);
  doc.AddMember(StringRef(KEY_AUTO_LOGIN), auto_login_, allocator);
  doc.AddMember(StringRef(KEY_AUTO_MIN), auto_min_, allocator);
  doc.AddMember(StringRef(KEY_SERVER_IP), StringRef(server_ip_.c_str()), allocator);
  doc.AddMember(StringRef(KEY_MAC_ADDRESS), StringRef(mac_address_.c_str()), allocator);
  StringBuffer buffer;
  PrettyWriter<StringBuffer> writer(buffer);
  doc.Accept(writer);
  ofstream fout(CONFIG_PATH());
  fout << buffer.GetString() << endl;
  fout.close();
}

const std::string &Config::user() const {
  return user_;
}

const std::string &Config::password() const {
  return password_;
}

const std::string &Config::server_ip() const {
  return server_ip_;
}

const std::string &Config::mac_address() const {
  return mac_address_;
}

bool Config::auto_min() const {
  return auto_min_;
}

bool Config::remember_me() const {
  return remember_me_;
}

bool Config::auto_login() const {
  return auto_login_;
}

void Config::set_user(const std::string &user) {
  user_ = user;
}

void Config::set_password(const std::string &password) {
  password_ = password;
}

void Config::set_server_ip(const std::string &server_ip) {
  server_ip_ = server_ip;
}

void Config::set_mac_address(const std::string &mac_address) {
  mac_address_ = mac_address;
}

void Config::set_auto_min(bool auto_min) {
  auto_min_ = auto_min;
}

void Config::set_remember_me(bool remember_me) {
  remember_me_ = remember_me;
}

void Config::set_auto_login(bool auto_login) {
  auto_login_ = auto_login;
}


} //namespace drcomclient