#ifndef DECODE_ERROR_H
#define DECODE_ERROR_H

#include <string>

struct DecodeError {
  std::string rule_id;
  std::string field;
  std::string description;
};

#endif
