# PCIe TLP Header Structure (Transaction Layer)

A TLP consists of:

* Header: **3DW (12B) or 4DW (16B)**
* Optional Data Payload

Which header is used depends on:

* Address size (32 vs 64 bit)
* Whether data is present

---

# DWORD 0 — Format, Type, Length

### DW0 Bit Layout

```
31..29  Fmt
28..24  Type
23      Reserved
22..20  TC
19..16  Reserved
15      TD
14      EP
13..12  Attr
11..10  Reserved
9..0    Length (in DW)
```

---

## Fmt (Format) — bits [31:29]

Determines:

* Header size
* Whether payload exists

| Fmt    | Meaning        | Header | Has Data |
| ------ | -------------- | ------ | -------- |
| 000    | 3DW, no data   | 12B    | No       |
| 001    | 4DW, no data   | 16B    | No       |
| 010    | 3DW, with data | 12B    | Yes      |
| 011    | 4DW, with data | 16B    | Yes      |
| others | **Illegal**    | —      | —        |

### Malformed if:

* Fmt ≥ 100

---

## Type — bits [28:24]

Interpreted together with Fmt.

### Common Encodings

| Type  | Meaning                   |
| ----- | ------------------------- |
| 00000 | Memory Read (MRd)         |
| 00001 | Memory Write (MWr)        |
| 01010 | Completion (Cpl)          |
| 01011 | Completion w/ Data (CplD) |

⚠ But Completion vs Completion with Data must match Fmt.

### Examples:

* MRd must be **no data**
* MWr must be **with data**
* Cpl must be **no data**
* CplD must be **with data**

Mismatch → malformed.

---

## TC (Traffic Class) — bits [22:20]

Range:

* 0–7

Used for QoS.

Usually ignore for validation, but must be extracted.

---

## Attr — bits [13:12]

Two independent flags:

| Bit | Meaning          |
| --- | ---------------- |
| 13  | No Snoop         |
| 12  | Relaxed Ordering |

So values:

| Attr | Meaning       |
| ---- | ------------- |
| 00   | default       |
| 01   | RO            |
| 10   | No Snoop      |
| 11   | No Snoop + RO |

Your JSON:

```json
"attr": { "no_snoop": true, "relaxed_ordering": false }
```

---

## Length — bits [9:0]

Payload length in **DWORDs**.

Rules:

* For no-data packets: must be **0**
  * like : CplD / Cpl
* For data packets: must be ≥ 1
  * like MRd / MWr
  
Special case:

* Length=0 means **1024 DW** (PCIe spec quirk)

Malformed if:

* MWr / MRd has zero length

---

# DWORD 1 — Requester or Completer Info

Depends on packet type.

---

## For Requests (MRd, MWr)

### DW1 Layout

```
31..16 Requester ID
15..8  Tag
7..4   Last BE
3..0   First BE
```

---

### Requester ID

16-bit:

```
Bus[15:8] : Device[7:3] : Function[2:0]
```

Example:

```
0000:03:00.0
```

Used to match completions.

---

### Tag

8-bit ID.

Used to match:

* MRd → Cpl / CplD

Rules:

* Must be echoed in completion
* Multiple outstanding allowed with different tags

---

### Byte Enables

Which bytes in first/last DW are valid.

Often ignored in Phase 1, but should still be decoded.

Malformed if:

* All BE bits are zero

---

## For Completions (Cpl / CplD)

### DW1 Layout

```
31..16 Completer ID
15..13 Status
12     BCM
11..0  Byte Count
```

---

### Status

| Value  | Meaning                     |
| ------ | --------------------------- |
| 000    | Successful Completion (SC)  |
| 001    | Unsupported Request (UR)    |
| 010    | Configuration Request Retry |
| others | Reserved → malformed        |

---

### Byte Count

Number of valid data bytes returned.

Rules:

* Must be > 0 for CplD
* Must match request length

Mismatch → validation error (not malformed).

---

# DWORD 2 — Address or Lower Address

---

## For Requests

### DW2

* If 3DW header → bits [31:2] = Address[31:2]
* If 4DW header → upper address in DW2, lower in DW3

Rules:

* Must be naturally aligned
* For MRd/MWr: alignment must match payload size

Misalignment → validation error.

---

## For Completions

DW2 contains:

* Lower address bits of the request

Used for alignment checks.

---

# Payload

Only if Fmt indicates data.

Size:

```
Length × 4 bytes
```

Malformed if:

* Payload shorter than expected

---

# Decode Errors vs Validation Errors

This distinction is critical.

---

## ❌ Malformed (Decode Errors)

Detected during parsing:

| Error                        | Example                |
| ---------------------------- | ---------------------- |
| Illegal Fmt                  | Fmt = 101              |
| Header too short             | only 8 bytes           |
| MRd with data Fmt            | impossible combination |
| Length mismatch with payload | truncated packet       |
| Illegal status code          | Cpl status = 111       |

These go in:

```
is_malformed = true
decode_errors[]
```

These packets should NOT enter validator logic.

---

## ⚠ Validation Errors (Protocol Errors)

Detected after successful decode:

| Error                 | Example                      |
| --------------------- | ---------------------------- |
| Missing completion    | MRd never completed          |
| Unexpected completion | Cpl with no matching request |
| Duplicate completion  | same tag twice               |
| Wrong byte count      | partial data                 |
| Address misalignment  | violates spec                |

These are protocol rule violations.

---

# 🔍 Example Walkthrough (Conceptual)

Raw bytes:

```
00 00 00 04  03 00 0F 00  00 00 10 00
```

DW0:

* Fmt = 000 → 3DW no data
* Type = 00000 → MRd
* Length = 4 → ❌ illegal (MRd must have no data)

→ malformed

---

Another:

DW0:

* Fmt = 010 → 3DW with data
* Type = 00001 → MWr
* Length = 8 → OK

Then:

* Decode Requester ID
* Decode Tag
* Decode Address

Then check payload length.

---

# Mapping to Your JSON Fields

| JSON Field            | Source             |
| --------------------- | ------------------ |
| type                  | Fmt + Type         |
| header_fmt            | Fmt                |
| tc                    | DW0[22:20]         |
| attr.no_snoop         | DW0[13]            |
| attr.relaxed_ordering | DW0[12]            |
| requester_id          | DW1[31:16]         |
| completer_id          | DW1[31:16] for Cpl |
| tag                   | DW1[15:8]          |
| address               | DW2 / DW3          |
| length_dw             | DW0[9:0]           |
| byte_count            | DW1[11:0]          |
| status                | DW1[15:13]         |


---

### Scenario A: Request (Memory Write - MWr)

**Input String:**
`2500,TX,4000000105080A0F30000000`

**1. Parse CSV Prefix:**

- **index**: 1 (Incremented from previous)
- **timestamp_ns**: `2500`
- **direction**: `"TX"`

**2. Parse TLP Hex (`4000000105080A0F30000000`):**

- **Split into DWs:**
- **DW0:** `40000001`
- **DW1:** `05080A0F`
- **DW2:** `30000000`

- **Parse DW0 (Common Fields):**
- `Fmt` (Bits 31:29): `010` → **"3DW"** (and implies Data exists).
- `Type` (Bits 28:24): `00000` → Combine with Fmt `010` → **"MWr"**.
- `TC` (Bits 22:20): `0`.
- `Attr` (Bits 18, 13:12): All 0 → `false`.
- `Length` (Bits 9:0): `1` → **1**.

- **Parse DW1 & DW2 (Request Specific Logic):**
- `Requester ID` (DW1 31:16): `0508` → Bus `05`, Dev `01` (`00001`), Func `0` → **"0000:05:01.0"**.
- `Tag` (DW1 15:8): `0A` → **10**.
- `Address` (DW2 31:0): `30000000` → **"0x30000000"**.

- **Set Nulls:** `completer_id`, `status`, `byte_count` are null for Requests.

**Final JSON Output:**

```json
{
  "index": 1,
  "timestamp_ns": 2500,
  "direction": "TX",
  "tlp": {
    "type": "MWr",
    "header_fmt": "3DW",
    "tc": 0,
    "attr": { "no_snoop": false, "relaxed_ordering": false },
    "requester_id": "0000:05:01.0",
    "completer_id": null,
    "tag": 10,
    "address": "0x30000000",
    "length_dw": 1,
    "byte_count": null,
    "status": null
  }
}
```

---

### Scenario B: Completion (Completion with Data - CplD)

**Input String:**
`3100,RX,4A0000010200200405080A00`

**1. Parse CSV Prefix:**

- **index**: 2
- **timestamp_ns**: `3100`
- **direction**: `"RX"`

**2. Parse TLP Hex (`4A0000010200200405080A00`):**

- **Split into DWs:**
- **DW0:** `4A000001`
- **DW1:** `02002004`
- **DW2:** `05080A00`

- **Parse DW0 (Common Fields):**
- `Fmt` (Bits 31:29): `010` → **"3DW"**.
- `Type` (Bits 28:24): `01010` → Combine with Fmt `010` → **"CplD"**.
- `Length` (Bits 9:0): `1` → **1**.

- **Parse DW1 & DW2 (Completion Specific Logic):**
- `Completer ID` (DW1 31:16): `0200` → Bus `02`, Dev `00`, Func `0` → **"0000:02:00.0"**.
- `Status` (DW1 15:13): Bits are `001` → **1** (Unsupported Request / UR). _Note: `20` hex is `0010 0000` binary. Bits 15:13 of the whole word correspond to the top 3 bits of `2` here._
- `Byte Count` (DW1 11:0): `004` → **4**.
- `Requester ID` (DW2 31:16): `0508` → Bus `05`, Dev `01`, Func `0` → **"0000:05:01.0"**.
- `Tag` (DW2 15:8): `0A` → **10**.

- **Set Nulls:** `address` is null for Completions.

**Final JSON Output:**

```json
{
  "index": 2,
  "timestamp_ns": 3100,
  "direction": "RX",
  "tlp": {
    "type": "CplD",
    "header_fmt": "3DW",
    "tc": 0,
    "attr": { "no_snoop": false, "relaxed_ordering": false },
    "requester_id": "0000:05:01.0",
    "completer_id": "0000:02:00.0",
    "tag": 10,
    "address": null,
    "length_dw": 1,
    "byte_count": 4,
    "status": 1
  }
}
```
---

Let's zoom in specifically on **DW1** of the Completion example to see exactly how we pull those numbers out.

The DW1 value is: **`02002004`** (Hex)

We need to look at this in **Binary** (32 bits) to see the boundaries.

### 1. Breakdown of `02002004`

Here is the hex converted to binary, bit by bit:

| Hex | `0` | `2` | `0` | `0` | `2` | `0` | `0` | `4` |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **Bits** | 31-28 | 27-24 | 23-20 | 19-16 | 15-12 | 11-8 | 7-4 | 3-0 |
| **Bin** | `0000` | `0010` | `0000` | `0000` | `0010` | `0000` | `0000` | `0100` |

---

### 2. Parsing Completer ID (Bits 31:16)

The Completer ID is the **first 16 bits** (the top half).
Looking at the table above, the top half is Hex `0200`.

**Binary:** `0000 0010 0000 0000`

We split this 16-bit chunk into Bus, Device, and Function:

* **Bus (Top 8 bits):** `0000 0010`
* Decimal/Hex: **2**


* **Device (Next 5 bits):** `00000`
* Decimal: **0**


* **Function (Bottom 3 bits):** `000`
* Decimal: **0**



**Result:** Bus 2, Device 0, Function 0.
**String:** `0000:02:00.0` (The leading `0000:` is the Domain, usually defaulted to 0).

---

### 3. Parsing Status (Bits 15:13)

The Status field is hidden inside the 5th hex digit (index 15-12).
In our value `02002004`, the 5th digit is **`2`**.

Let's look at bits 15 down to 12 (Hex digit `2`):
**Binary:** `0010`

* **Bit 15:** `0`
* **Bit 14:** `0`
* **Bit 13:** `1`
* **Bit 12:** `0` (This bit belongs to the "BCM" field, not Status)

We only want **Bits 15:13**.
Value: **`001`** (Binary) = **1** (Decimal).

**Interpretation of Status Code:**

* `000` (0): **SC** (Successful Completion)
* `001` (1): **UR** (Unsupported Request) <-- **This is our case**
* `010` (2): **CRS** (Configuration Request Retry Status)
* `100` (4): **CA** (Completer Abort)

### Summary Visualization

```text
Hex Value:      0    2    0    0    2    0    0    4
Binary:       0000 0010 0000 0000 0010 0000 0000 0100
              |-------| |---| |-| || |----------|
Field:           Bus     Dev  Fn  St    Byte Count
                 (2)     (0)  (0) (1)      (4)

```

* **Bus:** `00000010` = 2
* **Dev:** `00000` = 0
* **Fn:** `000` = 0
* **St (Status):** `001` = 1 (UR)
