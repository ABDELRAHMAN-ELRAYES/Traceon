#ifndef DECODE_ERROR
#define DECODE_ERROR

#include <string>

struct DecodeError {
  std::string rule_id;
  std::string field;
  std::string description;
};

#endif
