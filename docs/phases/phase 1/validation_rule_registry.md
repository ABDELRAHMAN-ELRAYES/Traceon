# Validation Rule Registry

The following validation rules are evaluated by the `ProtocolValidator` during and after trace processing. Each rule is associated with an error category that appears in the report output. All rules are evaluated against non-malformed TLPs only.

| Rule ID | Category | Trigger Condition | Description |
|---|---|---|---|
| VAL-001 | `UNEXPECTED_COMPLETION` | A `Cpl` or `CplD` is received with `(requester_id, tag)` not present in the outstanding-request table | Completion without a matching request |
| VAL-002 | `MISSING_COMPLETION` | An entry remains in the outstanding-request table after `finalize()` is called | MRd with no CplD before end-of-trace |
| VAL-003 | `DUPLICATE_COMPLETION` | A second `CplD` is received for a `(requester_id, tag)` that was already satisfied and removed | Duplicate completion |
| VAL-004 | `BYTE_COUNT_MISMATCH` | `CplD.byte_count` does not equal `MRd.length_dw × 4` (for non-split completions) | Returned byte count does not match requested length |
| VAL-005 | `ADDRESS_MISALIGNMENT` | `MRd` or `MWr` address is not naturally aligned to `length_dw × 4` bytes | Address alignment violation |
| VAL-006 | `TAG_COLLISION` | A new `MRd` is issued with `(requester_id, tag)` already present in the outstanding-request table | Tag reuse before prior request completes |
| VAL-007 | `INVALID_FIELD_VALUE` | `tc` value exceeds 7 (should be unreachable with 3-bit field, but defensive check is required) | Invalid traffic class value |
| VAL-008 | `INVALID_FIELD_VALUE` | TLP contains an illegal Length field value (e.g., zero for `MRd`/`MWr`/`CplD`, or non-zero for `Cpl`) | Invalid Length field value |

## 10.1 Rule Extensibility Contract

The validation rule registry is designed for extension without modification. Each rule is an independent, self-describing entity. To add a new validation rule, a developer creates a new rule object, assigns it the next available VAL-xxx identifier, and registers it with the rule registry. The `ProtocolValidator` discovers and evaluates all registered rules automatically. No modification to any other component — the decoder, the input layer, the reporting layer, or the orchestrator — is required.
