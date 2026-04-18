#ifndef PROTOCOL_VALIDATOR_H
#define PROTOCOL_VALIDATOR_H

#include "analyzer-engine/core/tlp/tlp.h"
#include "analyzer-engine/validator/validation_error.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

/**
 * the transaction key used to identify a transaction
 * shared unique attributes across transaction packets
 * - (requester_id, tag)
 */
struct TransactionKey {
  std::string requester_id;
  std::uint8_t tag;
  // Apply the equality for the map key by overloading the == operator
  bool operator==(const TransactionKey &key) const {
    return requester_id == key.requester_id && tag == key.tag;
  }
};

// Implemented the hash functor for the TransactionKey struct
namespace std {
template <> struct hash<TransactionKey> {
  size_t operator()(const TransactionKey &key) const noexcept {
    size_t h1 = std::hash<std::string>{}(key.requester_id);
    size_t h2 = std::hash<uint8_t>{}(key.tag);

    return h1 ^ (h2 << 1);
  }
};
} // namespace std

// the request that has been sent but not completed yet
struct OutstandingRequest {
  std::uint64_t packet_index;
  std::uint16_t length_dw;
  std::uint64_t timestamp_ns;
};

class ProtocolValidator {
private:
  // waiting table for requests that are not completed yet
  std::unordered_map<TransactionKey, OutstandingRequest> outstanding_requests_;

  // history of completed requests to detect duplicate completions
  std::unordered_set<TransactionKey> satisfied_requests_;

  // list of errors found during the validation process
  std::vector<ValidationError> errors_;

public:
  ProtocolValidator() = default;

  /**
   * @brief Process a single TLP and check for protocol violations
   * @param tlp The TLP to process
   */
  void process(const TLP &tlp);

  /**
   * @brief Finalize the validation process by adding any errors like the
   * uncompleted requests errors and return the full final errors list
   * @return A vector of validation errors
   */
  std::vector<ValidationError> finalize();

  /**
   * @brief Get the list of validation errors happened so far
   * @return A constant reference to the vector of validation errors
   */
  const std::vector<ValidationError> &errors() const;
};

#endif