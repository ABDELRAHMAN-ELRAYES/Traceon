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
