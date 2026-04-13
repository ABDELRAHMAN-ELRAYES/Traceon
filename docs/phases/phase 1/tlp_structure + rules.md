## **Key Fields of a TLP in JSON**

Each TLP in input has fields like:

```json
{
  "type": "MRd",
  "header_fmt": "3DW",
  "tc": 0,
  "attr": { "no_snoop": false, "relaxed_ordering": true },
  "requester_id": "0000:03:00.0",
  "completer_id": null,
  "tag": 2,
  "address": "0x000000003000",
  "length_dw": 16,
  "byte_count": null,
  "status": null
}
```

Let’s go field by field.

---

### **1. `type` — TLP Type**

**Values:**

* `"MRd"` — Memory Read Request
* `"MWr"` — Memory Write Request
* `"CplD"` — Completion with Data
* `"Cpl"` — Completion without Data
* Future extensions (CXL) could be `"CXL.cache"`, `"CXL.mem"`, `"CXL.io"`

**Behavior / Rules:**

* **MRd** must have a valid `requester_id`, `address`, `length_dw`. It expects a completion (`CplD`) with the same tag later.
* **MWr** must have `requester_id`, `address`, and data length. No completion with data is sent.
* **CplD** must match a previous MRd tag.
* **Cpl** is used for write completions or control completions.

**Valid Cases:**

* MRd → CplD with matching `tag`
* MWr → optionally Cpl (depending on protocol)

**Invalid/Error Cases:**

* MRd without a valid `address`
* CplD with `tag` that never appeared
* CplD with `byte_count` not matching requested length

Your analyzer should flag these.

---

### **2. `header_fmt` — Header Format**

**Values:**

* `"3DW"` — 3 Double Words header (standard for MRd, MWr, CplD small requests)
* `"4DW"` — 4 Double Words header (used when 64-bit addressing is required)

**Notes:**

* If a TLP uses 3DW header but has an address > 32-bit, it’s **invalid**.
* TLM or future extensions may require more DWs for extra metadata.

---

### **3. `tc` — Traffic Class**

**Values:**

* Integer `0–7` (3 bits in PCIe spec)
* Used for Quality-of-Service (QoS) and prioritization

**Notes for Analyzer:**

* Invalid value > 7 should be flagged
* Can be used in Phase 1 for logging QoS class

---

### **4. `attr` — Attributes**

```json
"attr": { "no_snoop": false, "relaxed_ordering": true }
```

#### **a) `no_snoop`**

* **true:** The request should bypass caches (used for I/O or DMA)
* **false:** Normal memory request (can be snooped)

**Error Cases:**

* `no_snoop` true on a request targeting memory with cacheable regions may cause ordering issues (your analyzer can log a warning)

#### **b) `relaxed_ordering`**

* **true:** Transaction can be reordered relative to other requests (higher throughput)
* **false:** Must maintain strict order
* **Invalid Cases:**

  * Relaxed ordering is not allowed for atomic or fence operations
  * Analyzer can flag ordering violations if a subsequent completion returns out of sequence for `relaxed_ordering=false`

---

### **5. `requester_id` / `completer_id`**

* **`requester_id`:** PCIe device sending the request (format: `bus:device.function`)
* **`completer_id`:** Device completing the request (null if request is outbound)

**Valid Cases:**

* MRd or MWr: requester_id = sender
* CplD or Cpl: completer_id = responder, matches MRd tag

**Invalid/Error Cases:**

* CplD with `completer_id=null` → invalid
* Mismatch in `completer_id` vs `requester_id` tag mapping

---

### **6. `tag`**

* Identifies transactions for matching requests and completions
* Must be **unique per outstanding request** per requester

**Error Cases:**

* Duplicate tag for a pending MRd → potential collision
* CplD with unmatched tag → missing request (violates PCIe spec)

---

### **7. `address`**

* Memory address in hexadecimal
* Required for MRd/MWr
* Cpl/CplD typically `null` (not needed)

**Error Cases:**

* Null or misaligned address for MRd/MWr → invalid TLP
* Address not DW-aligned (4 bytes for 32-bit DW) → misaligned

---

### **8. `length_dw` & `byte_count`**

* **`length_dw`:** Number of double words requested (MRd/MWr)
* **`byte_count`:** Number of bytes actually completed (CplD)

**Rules:**

* MRd length_dw > 0
* CplD byte_count = requested DW * 4 (unless partial completion)
* MWr length_dw > 0, Cpl optional

**Error Cases:**

* MRd with length_dw=0 → invalid
* CplD byte_count != requested length → protocol violation

---

### **9. `status`**

* Completion status: 
   * `"SC"` (Success)
   * `"UR"` (Unsupported Request)
   * `"CA"` (Completer Abort)
   * `"UNKNOWN"` (another state)

* Only for **Cpl/CplD**
* MRd/MWr status = null

**Invalid Cases:**

* MRd or MWr with non-null `status`
* CplD with invalid status string → analyzer can flag

---

## **Example Phase 1 Test Cases**

1. **Valid MRd → CplD**

   * MRd: index 0, length_dw=4, tag=0
   * CplD: index 1, byte_count=16, tag=0 → valid

2. **MRd missing completion**

   * MRd: index 2
   * No CplD → should log missing completion

3. **CplD with invalid tag**

   * Tag not sent by any MRd → analyzer flags error

4. **MWr with relaxed ordering**

   * Ensure ordering in GUI matches spec

5. **Misaligned address**

   * MRd with address=0x1003 (not DW-aligned) → invalid

6. **Duplicate tag**

   * MRd with tag=2 sent before previous request with same tag completed → log collision

7. **CplD byte_count mismatch**

   * Requested 16 DW, completed only 8 DW → log error

8. **Invalid TC**

   * TC>7 → analyzer logs warning

9. **Invalid TLP type**

   * Type not recognized → analyzer ignores or logs

---

### **Summary**

| Field                   | Valid Values        | Typical Phase 1 Checks                     |
| ----------------------- | ------------------- | ------------------------------------------ |
| `type`                  | MRd, MWr, CplD, Cpl | Exists, matches request/completion pairing |
| `header_fmt`            | 3DW, 4DW            | Address DW width matches header            |
| `tc`                    | 0–7                 | Flag out-of-range values                   |
| `attr.no_snoop`         | true/false          | Check cache rules                          |
| `attr.relaxed_ordering` | true/false          | Ensure order compliance                    |
| `requester_id`          | PCIe ID             | Non-null, matches completions              |
| `completer_id`          | PCIe ID or null     | Must match request tag for completions     |
| `tag`                   | 0–255               | Unique per outstanding request             |
| `address`               | Hex aligned         | Non-null for MRd/MWr, DW-aligned           |
| `length_dw`             | >0                  | Matches request/byte_count                 |
| `byte_count`            | >0                  | Matches request length in CplD             |
| `status`                | SC, UR, CA, TO      | Only for completions                       |

---

I can also **draw a diagram showing valid/invalid flows of MRd→CplD and MWr→Cpl**, with attributes highlighted, which will be **extremely useful for Phase 1 testing and your analyzer design**.

Do you want me to create that diagram next?
