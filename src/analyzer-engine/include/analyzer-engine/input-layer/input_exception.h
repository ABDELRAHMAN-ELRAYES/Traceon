#ifndef INPUT_LAYER_EXCEPTION
#define INPUT_LAYER_EXCEPTION

#include <stdexcept>
#include <string>

class TraceInputException : public std::runtime_error {

public:
  explicit TraceInputException(const std::string &message);
};

#endif