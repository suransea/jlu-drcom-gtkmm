//
// Created by sea on 12/23/18.
//

#ifndef JLU_DRCOM_MAC_ADDRESS_H
#define JLU_DRCOM_MAC_ADDRESS_H

#include <array>
#include <string>
#include <vector>

typedef unsigned char byte;

namespace DrcomClient {

class MacAddress {
public:
  static std::vector<MacAddress> all();

  MacAddress(const char *device_name, const std::array<byte, 6> &mac);

  // from format: "<mac_str>@<device-name>"
  explicit MacAddress(const std::string &str);

  MacAddress(const MacAddress &) = default;

  const std::array<byte, 6> &mac() const;

  const std::string &device_name() const;

  const std::string &mac_str() const;

  std::string to_string();

  byte operator[](std::size_t n) const;

protected:
  std::array<byte, 6> mac_;
  std::string device_name_;
  std::string mac_str_;
};

} // namespace DrcomClient

#endif // JLU_DRCOM_MAC_ADDRESS_H
