# Phase 1 — Decoding Rules Registry

This document summarizes all structural decoding rules implemented in the `PacketDecoder::decode()` method as of Phase 1. These rules are used to identify malformed Transaction Layer Packets (TLPs) during the initial decomposition of raw hexadecimal trace data.

## Structural Sanity Rules

| Rule Category | Description | Trigger Condition |
| :--- | :--- | :--- |
| **Minimum Length** | Ensures the packet has at least the minimum required header size. | Hex string length < 24 digits (3 Double Words). |
| **DW Alignment** | Checks if the packet data is truncated or contains partial words. | Hex string length is not a multiple of 8 digits (1 Double Word). |
| **4DW Boundary** | Verifies that a packet declared as 4DW actually has the 4th DW present. | Fmt indicates 4DW but the provided packet only contains 3 DWs. |

## Header Field Validation Rules

| Rule ID | Field | Description | Trigger Condition |
| :--- | :--- | :--- | :--- |
| **DEC-001** | `Fmt` | Validates the Format field [31:29] of DW0. | Fmt value is not in the supported range (000–011). |
| **DEC-002** | `Type` | Validates the TLP Type field [28:24] of DW0. | Type field does not correspond to a known PCIe TLP type (MRd, MWr, Cpl, CplD). |
| **DEC-003** | `TC` | Enforces valid Traffic Class priority. | TC field [22:20] is greater than the maximum allowed value of 7. |
| **DEC-008** | `Status` | Validates Completion Status [15:13] in DW1. | Status is not Successful (SC), Unsupported Request (UR), or Completer Abort (CA). |

## Logic & Cross-Validation Rules

| Rule Category | Description | Trigger Condition |
| :--- | :--- | :--- |
| **Fmt/Type Mismatch** | Ensures the packet's format bit matches the semantic type. | A data-carrying type (MWr, CplD) paired with a no-data Fmt, or vice-versa. |
| **Zero Length Check** | Validates payload length requirement for data types. | A payload-carrying TLP (MRd, MWr, CplD) has a Length field of 0. |
| **Non-Zero Length Check** | Enforces zero payload for no-data types. | A no-data TLP (like Cpl) has a non-zero value in the Length field. |

---

## Attribute Extraction Methodology

This section details exactly how each TLP attribute is extracted from the raw Double Words (DWs). All bit extraction follows the PCIe Base Specification bit-mapping via `Utils::extractBits(dw, lo, hi)`, which extracts bits from position `lo` to `hi` inclusive.

### Common Header Attributes (DW0)

All fields in this section are extracted from **DW0** regardless of TLP type.

| Attribute | Bit Range | Width | Extraction Expression | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Fmt** | `[31:29]` | 3 bits | `extractBits(dw0, 29, 31)` | Header format (3DW/4DW) and payload indicator. |
| **Type** | `[28:24]` | 5 bits | `extractBits(dw0, 24, 28)` | Identifies the specific transaction type (MRd, MWr, Cpl, CplD). |
| **Traffic Class (TC)** | `[22:20]` | 3 bits | `extractBits(dw0, 20, 22)` | QoS prioritization [0–7]. |
| **No Snoop** | `[13]` | 1 bit | `extractBits(dw0, 13, 13)` | Attribute bit for cache coherency management. |
| **Relaxed Ordering** | `[12]` | 1 bit | `extractBits(dw0, 12, 12)` | Attribute bit for memory ordering relaxation. |
| **Length** | `[9:0]` | 10 bits | `extractBits(dw0, 0, 9)` | Payload size in Double Words. |

### Packet-Type Branching Rule

After DW0 is decoded, the remaining DW fields are extracted conditionally based on the resolved TLP type. Two mutually exclusive branches apply:

| Branch | Condition | DWs Used |
| :--- | :--- | :--- |
| **Request path** | `type == MRd` or `type == MWr` | DW1, DW2, and optionally DW3 (for 4DW) |
| **Completion path** | `type == Cpl` or `type == CplD` | DW1 and DW2 |

### Memory Request Attributes (MRd / MWr)

These fields are extracted only when the TLP type falls into the **Request path**.

| Attribute | Source | Bit Range | Width | Extraction Expression | Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Requester ID (raw)** | DW1 | `[31:16]` | 16 bits | `extractBits(dw1, 16, 31)` | Raw 16-bit BDF identifier of the initiator. Passed to `translateBdfId()`. |
| **Tag** | DW1 | `[15:8]` | 8 bits | `extractBits(dw1, 8, 15)` | Transaction identifier for outstanding request tracking. |
| **Address (3DW)** | DW2 | `[31:2]` | 30 bits | `dw2 & 0xFFFFFFFCu` | 32-bit target address. The lowest 2 bits are always masked to zero. |
| **Address (4DW) — upper** | DW2 | `[31:0]` | 32 bits | `(uint64_t)dw2 << 32` | Upper 32 bits of the 64-bit address (bits [63:32]). |
| **Address (4DW) — lower** | DW3 | `[31:2]` | 30 bits | `(uint64_t)dw3 & 0xFFFFFFFCu` | Lower 32 bits of the 64-bit address. The lowest 2 bits are always masked to zero. |



### Completion Attributes (Cpl / CplD)

These fields are extracted only when the TLP type falls into the **Completion path**.

| Attribute | Source | Bit Range | Width | Extraction Expression | Description |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Completer ID (raw)** | DW1 | `[31:16]` | 16 bits | `extractBits(dw1, 16, 31)` | Raw 16-bit BDF identifier of the responder. Passed to `translateBdfId()`. |
| **Status** | DW1 | `[15:13]` | 3 bits | `extractBits(dw1, 13, 15)` | Completion status code (SC, UR, CA). |
| **Byte Count** | DW1 | `[11:0]` | 12 bits | `extractBits(dw1, 0, 11)` | Total remaining bytes for the transaction. |
| **Requester ID (raw)** | DW2 | `[31:16]` | 16 bits | `extractBits(dw2, 16, 31)` | Raw 16-bit BDF identifier of the original requester. Passed to `translateBdfId()`. |
| **Tag** | DW2 | `[15:8]` | 8 bits | `extractBits(dw2, 8, 15)` | Tag from the original request, used to match this completion to its request. |

### BDF Identifier Decomposition (`translateBdfId`)

Any raw 16-bit ID extracted above (Requester ID or Completer ID) is translated into a human-readable `Domain:Bus:Device.Function` string by `translateBdfId()`. The decomposition is as follows:

| Sub-field | Bit Range of Raw 16-bit ID | Width | Extraction Expression | Output Format |
| :--- | :--- | :--- | :--- | :--- |
| **Domain** | — | — | Hardcoded constant | Always `0000` |
| **Bus** | `[15:8]` | 8 bits | `(rawId >> 8) & 0xFF` | 2-digit hex (e.g., `0a`) |
| **Device** | `[7:3]` | 5 bits | `(rawId >> 3) & 0x1F` | 2-digit hex (e.g., `1f`) |
| **Function** | `[2:0]` | 3 bits | `rawId & 0x07` | 1-digit hex (e.g., `3`) |

The resulting string follows the standard Linux PCI notation: `0000:BB:DD.F` (e.g., `0000:0a:1f.3`).

---

## Implementation Reference
Every violation detected by these rules results in the `TLP.m_isMalformed` flag being set to `true`, and a human-readable description being appended to the `TLP.m_decodeErrors` collection.