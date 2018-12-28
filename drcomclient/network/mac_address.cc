//
// Created by sea on 12/23/18.
//

#include "mac_address.h"

#include <cstring>

#include <sys/ioctl.h>
#include <net/if.h>

namespace drcomclient {

const byte *MacAddress::mac() const {
  return mac_;
}

const std::string &MacAddress::device_name() const {
  return device_name_;
}

const std::string &MacAddress::mac_str() const {
  return mac_str_;
}

MacAddress::MacAddress(const char *device_name, const char *mac)
    : device_name_(device_name) {
  this->mac_[0] = (byte) mac[0];
  this->mac_[1] = (byte) mac[1];
  this->mac_[2] = (byte) mac[2];
  this->mac_[3] = (byte) mac[3];
  this->mac_[4] = (byte) mac[4];
  this->mac_[5] = (byte) mac[5];
  auto *str = new char[32];
  sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X", this->mac_[0], this->mac_[1], this->mac_[2],
          this->mac_[3], this->mac_[4], this->mac_[5]);
  mac_str_ = str;
  delete[] str;
}

std::vector<MacAddress> MacAddress::get_macs() {
  std::vector<MacAddress> address;
  const int MAX_INTERFACES = 16;
  struct ifreq buf[MAX_INTERFACES];
  struct ifconf ifc{};
  int fd;
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)return address;
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = (caddr_t) buf;
  if (ioctl(fd, SIOCGIFCONF, (char *) &ifc))return address;
  int count = ifc.ifc_len / sizeof(struct ifreq);
  for (int i = 0; i < count; ++i) {
    if (ioctl(fd, SIOCGIFHWADDR, (char *) &buf[i])) continue;
    address.emplace_back(buf[i].ifr_name, buf[i].ifr_hwaddr.sa_data);
  }
  return std::move(address);
}

MacAddress::MacAddress(const MacAddress &src)
    : device_name_(src.device_name_),
      mac_str_(src.mac_str_) {
  this->mac_[0] = src.mac_[0];
  this->mac_[1] = src.mac_[1];
  this->mac_[2] = src.mac_[2];
  this->mac_[3] = src.mac_[3];
  this->mac_[4] = src.mac_[4];
  this->mac_[5] = src.mac_[5];
}

MacAddress::MacAddress(const std::string &str) {
  for (int i = 0; i < 6; ++i) {
    mac_[i] = (byte) strtoul(str.c_str() + i * 3, nullptr, 16);
  }
  mac_str_ = str.substr(0, 17);
  device_name_ = str.substr(18, str.length() - 18);
}

std::string MacAddress::to_string() {
  return mac_str_ + "@" + device_name_;
}

} //namespace drcomclient