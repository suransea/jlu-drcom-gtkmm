//
// Created by sea on 12/23/18.
//

#include "mac_address.h"

#include <cstring>

#include <net/if.h>
#include <sys/ioctl.h>

namespace DrcomClient {

std::vector<MacAddress> MacAddress::all() {
  std::vector<MacAddress> address = {};
  const int MAX_INTERFACES = 16;
  struct ifreq buf[MAX_INTERFACES]{};
  struct ifconf ifc{};
  int fd;
  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    return address;
  ifc.ifc_len = sizeof(buf);
  ifc.ifc_buf = (caddr_t)buf;
  if (ioctl(fd, SIOCGIFCONF, (char *)&ifc))
    return address;
  int count = ifc.ifc_len / sizeof(struct ifreq);
  for (int i = 0; i < count; ++i) {
    if (ioctl(fd, SIOCGIFHWADDR, (char *)&buf[i]))
      continue;
    address.emplace_back(buf[i].ifr_name, buf[i].ifr_hwaddr.sa_data);
  }
  return std::move(address);
}

MacAddress::MacAddress(const char *device_name, const std::array<byte, 6> &mac)
    : device_name_(device_name), mac_(mac) {
  char mac_str[18];
  sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", this->mac_[0],
          this->mac_[1], this->mac_[2], this->mac_[3], this->mac_[4],
          this->mac_[5]);
  mac_str_ = mac_str;
}

MacAddress::MacAddress(const std::string &str) : mac_() {
  for (int i = 0; i < 6; ++i) {
    mac_[i] = static_cast<byte>(strtoul(str.c_str() + i * 3, nullptr, 16));
  }
  mac_str_ = str.substr(0, 17);
  device_name_ = str.substr(18, str.length() - 18);
}

const std::array<byte, 6> &MacAddress::mac() const { return mac_; }

const std::string &MacAddress::device_name() const { return device_name_; }

const std::string &MacAddress::mac_str() const { return mac_str_; }

std::string MacAddress::to_string() { return mac_str_ + "@" + device_name_; }

byte MacAddress::operator[](std::size_t n) const { return mac_[n]; }

} // namespace DrcomClient