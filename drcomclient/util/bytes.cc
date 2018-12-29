//
// Created by sea on 12/25/18.
//

#include <sstream>
#include <iomanip>

#include "bytes.h"

namespace drcomclient {

std::string bytes_to_string(byte *data, const std::size_t len) {
  using namespace std;
  stringstream ss;
  ss << "[bytes]";
  for (int i = 0; i < len; ++i) {
    ss << " " << setiosflags(ios::uppercase) << hex << (int) data[i];
  }
  string str;
  getline(ss, str);
  return str;
}

void checksum(const byte *data, const std::size_t len, byte *out) {
  auto sum = new byte[4];
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
    auto tmp = new byte[4]{0x00};
    for (int j = 3; j >= 0 && i < len; --j) {
      tmp[j] = data[i++];
    }
    for (int j = 0; j < 4; j++) {
      sum[j] ^= tmp[j];
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
    out[j] = (byte) (num & (byte) 0xff);
    num >>= 8;
  }
}

} //namespace drcomclient