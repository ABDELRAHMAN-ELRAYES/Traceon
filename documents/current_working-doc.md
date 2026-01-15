## STEP 1 — Freeze Your Internal TLP Model

You must define:

### What fields does **every decoded TLP object** contain?

Regardless of type, you will always need:

### Core Identity

* timestamp
* direction
* original packet index

### Header Fields

* fmt
* type
* length (DW)
* requester ID
* tag

### Address / Routing

* address (if applicable)
* completer ID (if applicable)

### Payload Info

* has_data
* byte_count (for completions)

### Decode Status

Very important:

* `is_malformed`
* `decode_errors[]`

Because:

> Decode failure ≠ protocol violation

Malformed packets should still flow into validator.

---

## STEP 2 — Define TLP Classification Early

Every TLP must be classified into:

* MRd
* MWr
* Cpl
* CplD
* Unsupported / Reserved

Because validation rules depend on class:

| TLP Type | Validator Needs         |
| -------- | ----------------------- |
| MRd      | expect completion       |
| MWr      | no completion           |
| Cpl      | must match request      |
| CplD     | must match + data rules |

So decoding output must already include:

> **transaction_category**

---

## STEP 3 — Design Validation State Tables (On Paper)

Before coding, define:

### What state must be tracked?

At minimum:

### Outstanding Request Table

Keyed by:

* requester_id
* tag

Stores:

* request type
* address
* length
* timestamp

### Completion Matching Logic

When completion arrives:

* does matching request exist?
* is byte count correct?
* is it duplicate?

### End-of-Trace Check

After file ends:

* any requests still pending → error

You should literally write:

> “Validator maintains map<RequestKey, RequestInfo>”

This becomes your protocol brain.

---

## STEP 4 — Define Error Taxonomy Now

Before implementation, define exact error classes:

### Structural (Decode Layer)

* invalid header length
* unsupported fmt/type
* truncated packet

### Protocol Violations

* completion without request
* missing completion
* tag reuse while outstanding
* misaligned address
* invalid length

Each error must include:

* packet index
* rule name
* description

This prevents messy ad-hoc errors later.

