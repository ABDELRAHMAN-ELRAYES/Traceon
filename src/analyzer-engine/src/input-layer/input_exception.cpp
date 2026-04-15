#include "analyzer-engine/input-layer/input_exception.h"

TraceInputException::TraceInputException(const std::string &message)
    : std::runtime_error(message) {}
