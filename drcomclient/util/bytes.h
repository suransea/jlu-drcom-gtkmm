//
// Created by sea on 12/25/18.
//

#ifndef JLU_DRCOM_BYTES_H
#define JLU_DRCOM_BYTES_H

#include <string>

namespace drcomclient {

typedef unsigned char byte;

std::string bytes_to_string(byte *data, size_t len);

} //namespace drcomclient

#endif //JLU_DRCOM_BYTES_H
