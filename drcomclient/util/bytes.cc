//
// Created by sea on 12/25/18.
//

#include "bytes.h"

namespace drcomclient {

std::string bytes_to_string(byte *data, const size_t len) {
  std::string str;
  str.append("[bytes]");
  for (int i = 0; i < len; ++i) {
    str.append(" ");
    str.append(std::to_string((int) data[i]));
  }
  return str;
}

} //namespace drcomclient