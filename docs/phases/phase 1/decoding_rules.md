# Phase 1 — Decoding Rules Registry 

## Table of Contents

1. [Double Word Structure](#1-double-word-structure)
2. [Fmt Field — 3-bit Header Format](#2-fmt-field--3-bit-header-format)
3. [Type Field — 5-bit TLP Type](#3-type-field--5-bit-tlp-type)
4. [Fmt × Type Combined Decoding](#4-fmt--type-combined-decoding)
5. [Traffic Class (TC)](#5-traffic-class-tc)
6. [Attribute Bits — No Snoop & Relaxed Ordering](#6-attribute-bits--no-snoop--relaxed-ordering)
7. [Length Field](#7-length-field)
8. [Memory Request Fields (MRd / MWr)](#8-memory-request-fields-mrd--mwr)
9. [Completion Fields (Cpl / CplD)](#9-completion-fields-cpl--cpld)
10. [Completion Status Codes](#10-completion-status-codes)
11. [BDF Identifier Decomposition](#11-bdf-identifier-decomposition)
12. [Structural Sanity Checks](#12-structural-sanity-checks)
13. [Cross-Validation Rules](#13-cross-validation-rules)

---

## 1. Double Word Structure

A PCIe TLP is a sequence of **Double Words (DWs)**, where each DW is **32 bits wide** (8 hexadecimal digits). The minimum legal TLP is **3 DWs (96 bits / 24 hex digits)**. A 4DW request header is **4 DWs (128 bits / 32 hex digits)**.

```
Raw hex string:  "4000000100010000DEADBEEF"
                 |------||------||------|
                   DW0     DW1     DW2
```

All bit positions follow the PCIe Base Specification convention:
- **Bit 31** is the Most Significant Bit (MSB) of a DW.
- **Bit 0** is the Least Significant Bit (LSB).
- `extractBits(dw, lo, hi)` extracts bits `[hi:lo]` inclusive.

---

## 2. Fmt Field — 3-bit Header Format

**Location:** DW0 bits `[31:29]`  
**Extraction:** `extractBits(dw0, 29, 31)`  
**Width:** 3 bits → 8 possible raw values (0–7)

The `Fmt` field simultaneously encodes **two pieces of information**:
1. Whether the header is **3DW or 4DW** (determines address width for requests).
2. Whether the TLP **carries a data payload** or not.

### Legal Fmt Encodings

| Raw Binary | Raw Decimal | Header Size | Has Data Payload | Symbolic Name        |
|:----------:|:-----------:|:-----------:|:----------------:|:---------------------|
| `000`      | 0           | 3 DW        | No               | `FMT_3DW_WITHOUT_DATA` |
| `001`      | 1           | 4 DW        | No               | `FMT_4DW_WITHOUT_DATA` |
| `010`      | 2           | 3 DW        | Yes              | `FMT_3DW_WITH_DATA`    |
| `011`      | 3           | 4 DW        | Yes              | `FMT_4DW_WITH_DATA`    |
| `100`–`111`| 4–7         | —           | —                | **ILLEGAL → DEC-001**  |

### How to Determine 3DW vs 4DW

```
if (Fmt == 000 OR Fmt == 010)  →  3DW header  →  32-bit address in DW2
if (Fmt == 001 OR Fmt == 011)  →  4DW header  →  64-bit address spanning DW2 (upper) + DW3 (lower)
```

> **Rule DEC-001:** Any `Fmt` value in the range `100`–`111` (decimal 4–7) is illegal and marks the TLP as malformed.

---

## 3. Type Field — 5-bit TLP Type

**Location:** DW0 bits `[28:24]`  
**Extraction:** `extractBits(dw0, 24, 28)`  
**Width:** 5 bits → 32 possible raw values (0–31)

The `Type` field identifies the transaction category. The decoder supports four types:

### Supported Type Encodings

| Raw Binary  | Raw Decimal | TLP Type | Description                        |
|:-----------:|:-----------:|:--------:|:-----------------------------------|
| `00000`     | 0           | `MRd`    | Memory Read Request (no data)      |
| `00001`     | 1           | `MWr`    | Memory Write Request (with data)   |
| `01010`     | 10          | `Cpl`    | Completion without Data            |
| `01011`     | 11          | `CplD`   | Completion with Data               |
| All others  | —           | —        | **UNKNOWN → DEC-002**              |

> **Rule DEC-002:** Any raw `Type` value not in the table above is unsupported and marks the TLP as malformed.

---

## 4. Fmt × Type Combined Decoding

`Fmt` and `Type` are **not independent** — their `hasData` and data-requirement attributes must agree. The cross-validation logic is:

| TLP Type | Requires Data? | Legal Fmt Values     | Illegal Fmt Values   |
|:--------:|:--------------:|:--------------------:|:--------------------:|
| `MRd`    | **No**         | `000`, `001`         | `010`, `011`         |
| `MWr`    | **Yes**        | `010`, `011`         | `000`, `001`         |
| `Cpl`    | **No**         | `000` (3DW only)     | `010`, `011`, `001`  |
| `CplD`   | **Yes**        | `010` (3DW only)     | `000`, `001`, `011`  |

### Decision Logic

```
fmtHasData   = (Fmt == 010 OR Fmt == 011)
typeNeedsData = (Type == MWr OR Type == CplD)
typeNeedsNoData = (Type == MRd OR Type == Cpl)

if typeNeedsData AND NOT fmtHasData   → MALFORMED (DEC-002, Fmt/Type mismatch)
if typeNeedsNoData AND fmtHasData     → MALFORMED (DEC-002, Fmt/Type mismatch)
```

### Full Combined Truth Table (supported types only)

| Fmt (raw) | Type (raw) | Valid? | Decoded Meaning                        |
|:---------:|:----------:|:------:|:---------------------------------------|
| `000`     | `00000`    | ✅     | 3DW Memory Read (32-bit address)       |
| `001`     | `00000`    | ✅     | 4DW Memory Read (64-bit address)       |
| `010`     | `00001`    | ✅     | 3DW Memory Write + payload             |
| `011`     | `00001`    | ✅     | 4DW Memory Write + payload             |
| `000`     | `01010`    | ✅     | 3DW Completion, no data                |
| `010`     | `01011`    | ✅     | 3DW Completion with data               |
| `010`     | `00000`    | ❌     | MRd cannot carry data                  |
| `000`     | `00001`    | ❌     | MWr must carry data                    |

---

## 5. Traffic Class (TC)

**Location:** DW0 bits `[22:20]`  
**Extraction:** `extractBits(dw0, 20, 22)`  
**Width:** 3 bits → values 0–7

TC controls QoS prioritization across the PCIe fabric. All 8 values are legal.

| TC Value | Priority     | Typical Use                        |
|:--------:|:------------:|:-----------------------------------|
| 0        | Default/Low  | Best-effort traffic (default)      |
| 1–6      | Escalating   | Application-defined classes        |
| 7        | Highest      | Isochronous / real-time traffic    |

> **Rule DEC-003:** TC > 7 is impossible with 3-bit extraction (max is 7), but if `extractBits` is called incorrectly on a wider field, values > 7 could appear and must be rejected.

---

## 6. Attribute Bits — No Snoop & Relaxed Ordering

Both are single-bit flags extracted from DW0.

| Attribute         | Bit  | Extraction                   | Meaning when SET (1)                                     |
|:------------------|:----:|:----------------------------:|:---------------------------------------------------------|
| **Relaxed Ordering** | `[12]` | `extractBits(dw0, 12, 12)` | Producer-Consumer ordering model is relaxed; packet may be reordered past older writes. |
| **No Snoop**        | `[13]` | `extractBits(dw0, 13, 13)` | Destination data is not expected to be in CPU caches; snooping the cache hierarchy is skipped. |

These bits are informational — there are no error rules for their values.

---

## 7. Length Field

**Location:** DW0 bits `[9:0]`  
**Extraction:** `extractBits(dw0, 0, 9)`  
**Width:** 10 bits → values 0–1023 (DW units)

The `Length` field encodes the **payload size in Double Words**.

> **Special encoding:** `Length == 0` encodes **1024 DWs** (4096 bytes) per the PCIe Base Specification. However, this decoder treats `Length == 0` as an error for simplicity.

### Validation Rules

| TLP Type      | Length == 0 | Length > 0 | Rule             |
|:-------------:|:-----------:|:----------:|:-----------------|
| `MRd`         | ❌ MALFORMED | ✅         | DEC-004          |
| `MWr`         | ❌ MALFORMED | ✅         | DEC-004          |
| `CplD`        | ❌ MALFORMED | ✅         | DEC-004          |
| `Cpl`         | ✅          | ❌ MALFORMED | DEC-004        |

---

## 8. Memory Request Fields (MRd / MWr)

These fields are extracted **only** when `Type == MRd` or `Type == MWr`.

### DW1 Fields

| Attribute       | Source | Bits      | Extraction                    | Description                                  |
|:----------------|:------:|:---------:|:-----------------------------:|:---------------------------------------------|
| **Requester ID**| DW1    | `[31:16]` | `extractBits(dw1, 16, 31)`    | 16-bit BDF of the initiating device          |
| **Tag**         | DW1    | `[15:8]`  | `extractBits(dw1, 8, 15)`     | 8-bit transaction tag for in-flight tracking |
| *(reserved)*    | DW1    | `[7:0]`   | —                             | Last Byte Enable / First Byte Enable (unused by this decoder) |

### DW2 / DW3 — Address Fields

#### 3DW Request (32-bit address)

```
DW2[31:2]  →  30 significant address bits
DW2[1:0]   →  always 0 (DW alignment, masked off)

address = dw2 & 0xFFFFFFFCu    // mask lowest 2 bits
```

The lowest 2 bits are **structurally required to be zero** (DW alignment). If they are non-zero, the address is misaligned → **Rule DEC-006**.

#### 4DW Request (64-bit address)

```
DW2[31:0]  →  upper 32 bits  [63:32]
DW3[31:2]  →  lower 30 significant bits  [31:2]
DW3[1:0]   →  always 0 (DW alignment, masked off)

address = ((uint64_t)dw2 << 32) | ((uint64_t)dw3 & 0xFFFFFFFCu)
```

The alignment check applies to DW3's lowest 2 bits → **Rule DEC-007**.

> **Rule DEC-005 (4DW):** If `Fmt` indicates 4DW but fewer than 4 DWs are present in the packet, the packet is malformed.

---

## 9. Completion Fields (Cpl / CplD)

These fields are extracted **only** when `Type == Cpl` or `Type == CplD`.

### DW1 Fields

| Attribute       | Source | Bits      | Extraction                    | Description                                  |
|:----------------|:------:|:---------:|:-----------------------------:|:---------------------------------------------|
| **Completer ID**| DW1    | `[31:16]` | `extractBits(dw1, 16, 31)`    | 16-bit BDF of the device sending the completion |
| **Status**      | DW1    | `[15:13]` | `extractBits(dw1, 13, 15)`    | 3-bit completion status code (see Section 10) |
| **BCM**         | DW1    | `[12]`    | —                             | Byte Count Modified (not decoded by this decoder) |
| **Byte Count**  | DW1    | `[11:0]`  | `extractBits(dw1, 0, 11)`     | Total remaining bytes in the split transaction |

### DW2 Fields

| Attribute       | Source | Bits      | Extraction                    | Description                                  |
|:----------------|:------:|:---------:|:-----------------------------:|:---------------------------------------------|
| **Requester ID**| DW2    | `[31:16]` | `extractBits(dw2, 16, 31)`    | 16-bit BDF of the **original** requester     |
| **Tag**         | DW2    | `[15:8]`  | `extractBits(dw2, 8, 15)`     | Tag from the original request (used for matching) |
| **Lower Address**| DW2   | `[6:0]`   | —                             | Byte offset within the first data DW (not decoded here) |

---

## 10. Completion Status Codes

**Location:** DW1 bits `[15:13]` (completion path only)  
**Extraction:** `extractBits(dw1, 13, 15)`  
**Width:** 3 bits → values 0–7

The status field tells the requester whether its request was fulfilled.

### Legal Status Encodings

| Raw Binary | Raw Decimal | Symbolic | Full Name              | Meaning                                                                                   |
|:----------:|:-----------:|:--------:|:-----------------------|:------------------------------------------------------------------------------------------|
| `000`      | 0           | `SC`     | Successful Completion  | The request was completed normally. Data is valid (for CplD).                            |
| `001`      | 1           | `UR`     | Unsupported Request    | The completer does not support the requested operation or address range.                  |
| `010`      | 2           | —        | Config Request Retry   | **Not decoded** — completer is temporarily busy (Config Space only).                     |
| `011`      | 3           | —        | Reserved               | **ILLEGAL → DEC-008**                                                                    |
| `100`      | 4           | `CA`     | Completer Abort        | The completer was able to handle the request but chose to abort it (e.g., access violation). |
| `101`–`111`| 5–7         | —        | Reserved               | **ILLEGAL → DEC-008**                                                                    |

> **Rule DEC-008:** Any status value not mapped to `SC (0)`, `UR (1)`, or `CA (4)` is illegal and marks the TLP as malformed. Note that values `2` and `3` are technically defined by the PCIe spec but are not supported by this decoder.

### Status Decision Flowchart

```
Raw Status = extractBits(dw1, 13, 15)

  == 0  →  SC   (Successful Completion)   ✅
  == 1  →  UR   (Unsupported Request)     ✅
  == 4  →  CA   (Completer Abort)         ✅
  else  →  UNKNOWN  →  DEC-008 error      ❌
```

---

## 11. BDF Identifier Decomposition

Any raw 16-bit ID (Requester ID or Completer ID) is decomposed into the standard Linux PCI notation `DDDD:BB:DD.F` by `translateBdfId()`.

### Bit Layout of the Raw 16-bit ID

```
Bit:   15  14  13  12  11  10   9   8 | 7   6   5   4   3 | 2   1   0
Field: [          Bus — 8 bits        ] [ Device — 5 bits ] [ Fn — 3b ]
```

### Extraction Table

| Sub-field  | Bits of raw 16-bit ID | Width  | Extraction C++                  | Output Format   | Example    |
|:-----------|:---------------------:|:------:|:--------------------------------:|:---------------:|:----------:|
| **Domain** | —                     | —      | Hardcoded `0`                   | 4-digit hex     | `0000`     |
| **Bus**    | `[15:8]`              | 8 bits | `(rawId >> 8) & 0xFF`           | 2-digit hex     | `0a`       |
| **Device** | `[7:3]`               | 5 bits | `(rawId >> 3) & 0x1F`           | 2-digit hex     | `1f`       |
| **Function** | `[2:0]`             | 3 bits | `rawId & 0x07`                  | 1-digit hex     | `3`        |

**Result format:** `0000:BB:DD.F`  
**Example:** raw ID `0x0AFB` → `0000:0a:1f.3`

---

## 12. Structural Sanity Checks

These checks run before any field extraction and cause an **immediate early return** if triggered.

| Check                   | Rule ID  | Trigger Condition                                        | Consequence              |
|:------------------------|:--------:|:---------------------------------------------------------|:-------------------------|
| Non-hex characters      | DEC-009  | Any character in the raw string is not `[0-9A-Fa-f]`    | Early return, malformed  |
| Minimum length          | DEC-005  | `rawBytesSize < 24` (fewer than 3 DWs)                   | Early return, malformed  |
| DW alignment            | DEC-005  | `rawBytesSize % 8 != 0` (truncated partial DW)           | Early return, malformed  |
| 4DW boundary            | DEC-005  | `Fmt == 4DW` but `dws.size() < 4`                        | Error added, malformed   |
| 3DW address alignment   | DEC-006  | `dw2 & 0x3 != 0` (lowest 2 bits set in 32-bit address)  | Error added, malformed   |
| 4DW address alignment   | DEC-007  | `dw3 & 0x3 != 0` (lowest 2 bits set in lower address DW)| Error added, malformed   |

---

## 13. Cross-Validation Rules

These rules check consistency **across** fields after individual field extraction.

| Rule                      | Fields Involved     | Condition That Triggers Error                                          |
|:--------------------------|:-------------------:|:-----------------------------------------------------------------------|
| **Fmt/Type Data Mismatch**| `Fmt`, `Type`       | Data-carrying Type (`MWr`, `CplD`) paired with no-data Fmt, or vice-versa |
| **Zero Length on Data TLP**| `Type`, `Length`  | `Type ∈ {MRd, MWr, CplD}` and `Length == 0`                          |
| **Non-Zero Length on No-Data TLP** | `Type`, `Length` | `Type ∈ {Cpl}` and `Length != 0`                             |
| **Invalid Completion Status** | `Status`        | Status not in `{0=SC, 1=UR, 4=CA}`                                    |

