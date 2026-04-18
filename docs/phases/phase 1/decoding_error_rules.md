# Phase 1 — Structural Decode Error Registry

**Document ID:** PCIE-DECODE-RULES-01  
**Project:** Traceon  
**Version:** 1.0.0  
**Status:** Approved  

This document serves as the authoritative registry for all structural decode errors detected by the Traceon `PacketDecoder` during Phase 1. These rules are applied to each `RawPacket` in isolation to identify malformed TLPs before they reach the validation engine.

---

## 1. Rule Definition Table

The following seven rules are evaluated during the decoding of each packet. A violation of any rule results in the `TLP.is_malformed` flag being set to `true`.

| Rule ID | Field | Triggering Condition | Description |
| :--- | :--- | :--- | :--- |
| **DEC-001** | `Fmt [31:29]` | The Fmt field value is one of the reserved combinations (`0b110` or `0b111`). | Detects use of PCIe format codes that are not defined in the base specification. |
| **DEC-002** | `Type [28:24]` | The Type field value, in combination with the Fmt field, does not correspond to a supported TLP. | Identifies unsupported transactions or Fmt/Type mismatches (e.g., MWr with no-data Fmt). |
| **DEC-003** | `payload_hex` | The total number of bytes is insufficient for the header size implied by the Fmt field, or length is not a multiple of 4 bytes. | Minimum requirements: 12 bytes for 3DW headers, 16 bytes for 4DW headers. |
| **DEC-004** | `Address [31:2]` | The 32-bit address in a 3DW Memory TLP is not aligned to a Double Word boundary. | The two least-significant bits ([1:0]) of the target address must be zero. |
| **DEC-005** | `Address [63:2]` | The 64-bit address in a 4DW Memory TLP is not aligned to a Double Word boundary. | The two least-significant bits ([1:0]) of the 64-bit target address must be zero. |
| **DEC-006** | `Status [15:13]` | The Completion Status field contains a value that is not a defined completion code. | Valid codes: SC (`000`), UR (`001`), or CA (`100`). All other values are invalid. |
| **DEC-007** | `payload_hex` | The `payload_hex` string contains characters that are not valid hexadecimal digits. | Ensures that numeric interpretation of the trace data is actually possible. |

---

## 2. Error Reporting Model

Every detected error is recorded as a `DecodeError` structure:

```cpp
struct DecodeError {
    std::string rule_id;      // e.g., "DEC-001"
    std::string field;        // Name of the affected TLP header field
    std::string description;  // Human-readable explanation of the violation
};
```

### 2.1 Malformed Packet Policy
As per the core architecture:
- A packet with at least one `DecodeError` is marked as **malformed**.
- Malformed packets carry whatever partial data could be successfully extracted up to the point of failure.
- **Filtering**: Malformed packets are logged in the analysis report but are **excluded** from the Protocol Validation Layer to prevent false-positive validation errors.

---

## 3. Implementation Status
These rules are fully implemented in `src/analyzer-engine/src/decoder/packet_decoder.cpp` and are verified against a canonical set of malformed trace files.
