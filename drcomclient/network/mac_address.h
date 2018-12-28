//
// Created by sea on 12/23/18.
//

#ifndef JLU_DRCOM_MAC_ADDRESS_H
#define JLU_DRCOM_MAC_ADDRESS_H

#include <vector>

#include "../util/bytes.h"

namespace drcomclient {

class MacAddress {
public:
  static std::vector<MacAddress> get_macs();

  MacAddress(const char *device_name, const char *mac);

  //from format: "<mac_str>@<device-name>"
  explicit MacAddress(const std::string &str);

  MacAddress(const MacAddress &);

  const byte *mac() const;

  const std::string &device_name() const;

  const std::string &mac_str() const;

  std::string to_string();

protected:
  byte mac_[6];
  std::string device_name_;
  std::string mac_str_;
};

} //namespace drcomclient

#endif //JLU_DRCOM_MAC_ADDRESS_H
