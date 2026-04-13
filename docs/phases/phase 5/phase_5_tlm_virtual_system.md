# Phase 5 — Transaction-Level Modeling and Virtual System Integration

**Document ID:** PCIE-SPEC-PHASE-05  
**Project:** Traceon  
**Version:** 2.0.0  
**Status:** Approved  
**Phase Dependencies:** Phase 1 (PCIE-SPEC-PHASE-01) — the `TLP` and `ValidationError` data structures are the authoritative source for all transaction reconstruction. Phase 4 (PCIE-SPEC-PHASE-04) — when live mode is active, the `TransactionMapper` consumes TLPs from the Consumer Thread output. Phases 2 and 3 are optional; their GUI timeline and batch scripting integrations are described in their respective sections of this document.  
**Consumed By:** Phase 7 (cross-domain transaction correlation).

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Glossary](#2-glossary)
3. [Phase Independence Guarantee](#3-phase-independence-guarantee)
4. [Functional Requirements](#4-functional-requirements)
5. [Non-Functional Requirements](#5-non-functional-requirements)
6. [Architectural Design](#6-architectural-design)
7. [Transaction Model](#7-transaction-model)
8. [Component Specifications](#8-component-specifications)
9. [Data Structures](#9-data-structures)
10. [Timing Model](#10-timing-model)
11. [System-Level Validation Rules](#11-system-level-validation-rules)
12. [Virtual System Representation](#12-virtual-system-representation)
13. [Reporting Extensions](#13-reporting-extensions)
14. [GUI Extensions](#14-gui-extensions)
15. [Testing Strategy](#15-testing-strategy)
16. [Risks and Mitigations](#16-risks-and-mitigations)
17. [Exit Criteria](#17-exit-criteria)

---

## 1. Purpose and Scope

### 1.1 Purpose

Phase 5 elevates the Traceon platform from **packet-level observation** to **transaction-level abstraction**. While Phase 1 observes and validates individual TLPs in isolation, Phase 5 groups related packets into logical transactions, tracks their complete lifecycle from initiation to resolution, and detects system-level behavioral violations that are only visible when packets are considered as causally connected sequences rather than independent events.

### 1.2 Objectives

- Group related sequences of TLPs into typed `Transaction` objects representing complete logical PCIe operations — for example, an MRd paired with its corresponding CplD.
- Track the lifecycle of each transaction through a set of defined states from initiation through completion, timeout, or error termination.
- Apply system-level validation rules that require multi-packet context and cannot be expressed within the single-packet scope of the Phase 1 engine.
- Infer and model the abstract virtual initiator and target devices participating in observed transactions.
- Annotate transactions with logical timing data enabling latency analysis and outlier detection.
- Extend the Traceon report schema with a transaction-level section, advancing the schema to version `2.0`.

### 1.3 In-Scope Capabilities

| Capability | Description |
|---|---|
| Transaction reconstruction | Grouping of TLPs into typed `Transaction` objects using tag and requester ID matching |
| Transaction lifecycle tracking | State machine tracking each transaction from creation through completion, timeout, or error |
| Logical timing annotation | Start time, end time, and derived latency recorded per transaction from TLP timestamps |
| System-level validation | Detection of violations requiring multi-packet context |
| Virtual component inference | Abstract initiator and target device models inferred from requester and completer IDs |
| Transaction reporting | Schema v2.0 report extension with per-transaction and virtual-component data |
| GUI timeline view | Optional horizontal timeline visualization of all transactions (requires Phase 2) |

### 1.4 Explicitly Out of Scope

| Excluded Capability | Rationale |
|---|---|
| Cycle-accurate timing analysis | Physical signal timing is not present in transaction-layer traces |
| RTL simulation or hardware co-simulation | Requires hardware-level integration outside software analyzer scope |
| Clock domain crossing analysis | Requires hardware topology information not present in TLP traces |
| Multi-protocol transaction correlation | Phase 7 responsibility |

---

## 2. Glossary

| Term | Definition |
|---|---|
| **Transaction** | A logical PCIe operation spanning one or more TLPs. Examples: an MRd and its CplD; an MWr with or without an optional Cpl. |
| **Transaction ID** | A system-assigned unique integer identifier for each `Transaction` object. Distinct from and independent of the PCIe `tag` field. |
| **Initiator** | The virtual component that originated the request TLP in a transaction, identified by the `requester_id` field. |
| **Target** | The virtual component that issued the completion TLP for a transaction, identified by the `completer_id` field. A target is associated with a transaction when its first completion is observed. |
| **Transaction Lifecycle** | The defined sequence of states: `PENDING` (request observed, no completion yet), `COMPLETE` (SC completion matched), `ERROR` (non-SC completion received), `TIMEOUT` (no completion within threshold), `INCOMPLETE` (no completion by end of trace). |
| **Transaction Map** | The internal data structure mapping `(requester_id, tag)` composite keys to active `Transaction` objects currently in `PENDING` state. |
| **Latency** | The difference in nanoseconds between the `timestamp_ns` of the first packet in a transaction and the `timestamp_ns` of its final completion packet. Null when timestamps are unavailable. |
| **System-Level Violation** | A protocol violation that is only detectable by observing multiple transactions together — not expressible within the per-packet scope of the Phase 1 validator. |
| **Virtual Component** | An abstract behavioral representation of a PCIe device inferred entirely from the identifiers observed in trace data. Has no physical or simulation counterpart. |
| **TLM** | Transaction-Level Modeling. An approach to system analysis that reasons about observable behavior in terms of logical transactions rather than raw packet sequences or signal waveforms. |

---

## 3. Phase Independence Guarantee

**Rule 1 — Additive layer only.** Phase 5 is purely additive. The Phase 1 packet-level analysis pipeline continues to execute exactly as specified in PCIE-SPEC-PHASE-01, regardless of whether Phase 5 is enabled.

**Rule 2 — Parallel consumer, not interceptor.** The `TransactionMapper` receives TLP objects from the Phase 1 pipeline output as a parallel consumer. It does not replace, wrap, modify, or intercept the `PacketDecoder` or `ProtocolValidator`. Their execution paths are unchanged.

**Rule 3 — Runtime disablability.** Phase 5 functionality shall be disablable through a configuration flag at runtime. When disabled, the system behaves identically to Phases 1 through 4, and the report does not contain a `transactions` section.

**Rule 4 — Failure isolation.** A failure in the `TransactionMapper` must not prevent the Phase 1 analysis report from being written. The Phase 1 report is always produced regardless of the outcome of the transaction modeling layer.

**Rule 5 — No modification of prior phases.** No Phase 1, 2, 3, or 4 source file is modified.

---

## 4. Functional Requirements

### 4.1 Transaction Reconstruction

| ID | Requirement |
|---|---|
| FR-TLM-01 | The system shall create a new `Transaction` object in `PENDING` state when an MRd TLP is observed, keyed by `(requester_id, tag)`. |
| FR-TLM-02 | The system shall associate an observed CplD or Cpl TLP with its corresponding `Transaction` by looking up `(requester_id, tag)` in the transaction map. |
| FR-TLM-03 | The system shall create a `Transaction` for each MWr TLP. If no Cpl is expected by the protocol, the transaction transitions directly to `COMPLETE`. If an optional Cpl is observed, it is associated and the transaction transitions to `COMPLETE` at that point. |
| FR-TLM-04 | Transactions that are still in `PENDING` state after all packets have been processed shall be transitioned to `INCOMPLETE` during finalization. |
| FR-TLM-05 | Malformed TLPs must not be associated with any transaction and must not influence transaction state in any way. |
| FR-TLM-06 | Each `Transaction` shall carry the indices of all constituent TLPs — the request index and all completion indices — enabling navigation from a transaction to its component packets in the report. |

### 4.2 Transaction Lifecycle

| ID | Requirement |
|---|---|
| FR-TLM-07 | Each transaction shall follow the state transitions defined in Section 7.2. No transaction may skip a defined transition or enter an undefined state. |
| FR-TLM-08 | A transaction that remains in `PENDING` state beyond a configurable timeout threshold, measured in logical nanoseconds using the `timestamp_ns` field of the most recently processed packet, shall transition to `TIMEOUT`. |
| FR-TLM-09 | A transaction whose first completion carries a non-SC status code — either UR (`001b`) or CA (`100b`) — shall transition to `ERROR`. |
| FR-TLM-10 | A transaction whose completion carries SC status and whose byte count is consistent with the original request shall transition to `COMPLETE`. |

### 4.3 Logical Timing

| ID | Requirement |
|---|---|
| FR-TLM-11 | Each transaction shall record the `timestamp_ns` of its originating request TLP as `start_time`. |
| FR-TLM-12 | Each transaction shall record the `timestamp_ns` of its final completion or timeout event as `end_time`. |
| FR-TLM-13 | Transaction latency shall be computed as `end_time - start_time` in nanoseconds and stored as the `latency_ns` field. |
| FR-TLM-14 | When `timestamp_ns` is zero for any relevant packet, indicating that timestamps were not present in the trace, `latency_ns` shall be recorded as `null`. |

### 4.4 System-Level Validation

| ID | Requirement |
|---|---|
| FR-TLM-15 | The system shall evaluate all system-level validation rules defined in Section 11 against the stream of transactions and produce `SystemViolation` records for each detected violation. |
| FR-TLM-16 | `SystemViolation` records shall be a separate, distinct type from Phase 1 `ValidationError` records. They shall appear in the report under the `transactions.system_violations` key and never in the `validation_errors` key. |
| FR-TLM-17 | System-level violations shall not suppress or replace any Phase 1 validation errors. Both populations coexist independently in the report. |

### 4.5 Virtual Component Modeling

| ID | Requirement |
|---|---|
| FR-TLM-18 | The system shall infer a virtual `Initiator` component for each distinct `requester_id` value observed across all request TLPs in the trace. |
| FR-TLM-19 | The system shall infer a virtual `Target` component for each distinct `completer_id` value observed across all completion TLPs in the trace. |
| FR-TLM-20 | Each virtual component shall carry aggregate statistics: total transaction count, completed transaction count, error transaction count, and incomplete transaction count. |

---

## 5. Non-Functional Requirements

| ID | Category | Requirement |
|---|---|---|
| NFR-TLM-01 | Determinism | Transaction reconstruction from an identical TLP sequence must always produce the same `Transaction` objects, in the same states, with the same violation set, across all runs and environments. |
| NFR-TLM-02 | Failure Isolation | An exception or error in the `TransactionMapper` must not propagate to the Phase 1 report writing path. The Phase 1 report must be written regardless of the outcome of the transaction modeling layer. |
| NFR-TLM-03 | Extensibility | Adding a new transaction type (for example, a configuration access transaction) must require changes only within the `TransactionMapper` and the system-level validation rule registry. No other component requires modification. |
| NFR-TLM-04 | Performance | The `TransactionMapper` must not increase the total analysis time by more than 20 percent compared to an equivalent Phase 1-only run on the same trace. |
| NFR-TLM-05 | Scalability | The transaction map must accommodate at least 65,536 simultaneously pending transactions without measurable performance degradation. |

---

## 6. Architectural Design

### 6.1 Phase 5 Layer Position

Phase 5 inserts the `TransactionMapper` as a **parallel downstream consumer** of the Phase 1 pipeline output. It does not intercept or modify the Phase 1 data flow in any way.

```
                     PacketDecoder
                          │
                          ▼
               ┌─────────────────┐
               │ProtocolValidator│  ← Phase 1 — unchanged
               └────────┬────────┘
                         │ TLP (non-malformed)
            ┌────────────┼────────────────┐
            │            │                │
            ▼            ▼                ▼
       ReportBuilder  TransactionMapper  (other future consumers)
       (Phase 1)      (Phase 5)
            │            │
            │     ┌──────┴──────────────────────┐
            │     │ VirtualComponentRegistry     │
            │     │ SystemViolationEngine        │
            │     └──────────────────────────────┘
            │            │
            └────────────┴──► Extended Report (schema v2.0)
```

### 6.2 Additive Report Extension

When Phase 5 is enabled, the report schema advances to version `2.0`. The `transactions` section and the `virtual_components` section are appended to the standard Phase 1 report structure. All existing Phase 1 fields remain unchanged and in their original positions. A consumer that checks `schema_version` and ignores sections it does not recognize will continue to process the report correctly.

---

## 7. Transaction Model

### 7.1 Transaction Types

| Type Name | Constituent TLPs | Completion Requirement |
|---|---|---|
| `MEMORY_READ` | One MRd + one or more CplD | Required; at least one CplD must be observed |
| `MEMORY_WRITE` | One MWr + optional Cpl | Not required; closes immediately if no Cpl expected |
| `MEMORY_WRITE_ACK` | One MWr + one Cpl | Required; Cpl explicitly observed |

### 7.2 Transaction State Machine

```
        ┌──────────┐
        │ CREATED  │  (upon observing request TLP)
        └────┬─────┘
             │
             ▼
        ┌──────────┐
        │ PENDING  │  (awaiting completion)
        └────┬─────┘
             │
     ┌───────┼───────────┬──────────────┐
     │       │           │              │
     ▼       ▼           ▼              ▼
 COMPLETE   ERROR     TIMEOUT      INCOMPLETE
 (SC cpl)  (UR/CA)  (threshold     (end of
            cpl)     exceeded)       trace)
```

State transitions are one-way. A transaction may not revert from a terminal state (`COMPLETE`, `ERROR`, `TIMEOUT`, `INCOMPLETE`) to any prior state.

---

## 8. Component Specifications

### 8.1 TransactionMapper

**Single Responsibility:** Accept TLP objects from the Phase 1 pipeline output, reconstruct `Transaction` objects, maintain the transaction map, evaluate the timeout threshold, and produce `SystemViolation` records for multi-packet violations.

**Behavioral Contract:**
- Provides a `process(TLP)` operation called once per non-malformed TLP, in the same order as Phase 1 processing.
- Provides a `finalize()` operation called once after all TLPs have been processed. Transitions all remaining `PENDING` transactions to `INCOMPLETE`. Evaluates any finalization-phase system-level rules.
- Runs within a try/catch block. Any internal exception is logged and the component's output for the current run is marked as partial. The Phase 1 report is unaffected.

### 8.2 VirtualComponentRegistry

**Single Responsibility:** Maintain a catalog of inferred `Initiator` and `Target` virtual components, updating their aggregate statistics as transactions complete.

**Behavioral Contract:**
- Registers a new `Initiator` the first time a `requester_id` is encountered across any request TLP.
- Registers a new `Target` the first time a `completer_id` is encountered across any completion TLP.
- Increments per-component counters on each transaction state transition.
- A device may appear simultaneously as both an Initiator and a Target if it originates requests and also satisfies completions for requests from other devices.

### 8.3 SystemViolationEngine

**Single Responsibility:** Evaluate system-level validation rules defined in Section 11 against the stream of transaction state transitions and produce `SystemViolation` records.

**Behavioral Contract:**
- Operates as a subscriber to `TransactionMapper` state transitions.
- Emits a `SystemViolation` immediately when a rule condition is satisfied.
- Each rule is independently registered and may be enabled or disabled through configuration.

---

## 9. Data Structures

### 9.1 Transaction

| Field | Type | Description |
|---|---|---|
| `txn_id` | `uint64_t` | System-assigned unique transaction identifier |
| `type` | `TransactionType` | Enumerated type: MEMORY_READ, MEMORY_WRITE, MEMORY_WRITE_ACK |
| `state` | `TransactionState` | Current lifecycle state |
| `requester_id` | `string` | PCIe Bus:Device.Function of the originating device |
| `completer_id` | `string` or null | PCIe Bus:Device.Function of the completing device; null until first completion observed |
| `tag` | `uint8_t` | PCIe transaction tag |
| `address` | `string` | Target memory address |
| `start_time_ns` | `uint64_t` | Timestamp of the originating request packet |
| `end_time_ns` | `uint64_t` or null | Timestamp of the final completion or timeout event |
| `latency_ns` | `uint64_t` or null | Derived latency; null if timestamps unavailable |
| `request_packet_index` | `uint64_t` | Index of the originating request TLP |
| `completion_packet_indices` | `uint64_t[]` | Indices of all associated completion TLPs |
| `completion_status` | `string` or null | Status of the most recent completion: SC, UR, CA, or null |
| `returned_byte_count` | `uint16_t` or null | Byte count from the CplD; null for non-data transactions |

### 9.2 SystemViolation

| Field | Type | Description |
|---|---|---|
| `rule_id` | `string` | The SYS-xxx identifier from the system-level rule registry |
| `category` | `string` | Symbolic violation category |
| `txn_id` | `uint64_t` | The transaction involved in the violation |
| `related_txn_id` | `uint64_t` or null | A second related transaction, where applicable |
| `description` | `string` | Human-readable description of the specific violation |

---

## 10. Timing Model

### 10.1 Logical Time

Phase 5 does not model physical time or cycle-accurate timing. It operates on **logical time**, derived exclusively from the `timestamp_ns` fields present in TLP objects. These values are whatever the trace source recorded; the platform makes no assumptions about their units, resolution, or monotonicity beyond what is specified here.

### 10.2 Timestamp Availability

When a trace was captured without timestamps — indicated by all `timestamp_ns` values being zero — the timing model is inactive. All `latency_ns` fields are reported as `null`, and timeout detection is disabled. The remainder of the transaction reconstruction and validation logic operates normally regardless of timestamp availability.

### 10.3 Timeout Threshold

The transaction timeout threshold is expressed in nanoseconds and is configurable via the `--txn-timeout-ns` CLI argument. Its default value is 100,000 nanoseconds. Setting the threshold to zero disables timeout detection entirely. The threshold is evaluated by comparing the `timestamp_ns` of the most recently processed packet against the `start_time_ns` of each pending transaction.

---

## 11. System-Level Validation Rules

| Rule ID | Category | Trigger Condition |
|---|---|---|
| SYS-001 | `TRANSACTION_TIMEOUT` | A transaction remains in `PENDING` state and the difference between the current packet's timestamp and the transaction's `start_time_ns` exceeds the configured timeout threshold. |
| SYS-002 | `TRANSACTION_INCOMPLETE` | A transaction is still in `PENDING` state when `finalize()` is called, meaning no completion was observed before the trace ended. |
| SYS-003 | `DUPLICATE_COMPLETION_ON_TRANSACTION` | A CplD is received for a transaction that has already been fully completed and removed from the active transaction map. |
| SYS-004 | `ERROR_COMPLETION` | A completion with UR or CA status is received, transitioning the transaction to `ERROR` state. |
| SYS-005 | `BYTE_COUNT_MISMATCH_ON_TRANSACTION` | The `returned_byte_count` in a CplD does not match the expected byte count derived from the `length_dw` field of the originating MRd. |

### 11.1 Rule Independence

Each system-level rule is independent of every other. A transaction may trigger multiple rules simultaneously. Rules are evaluated in the order in which their triggering conditions arise during stream processing, not in rule ID order.

---

## 12. Virtual System Representation

### 12.1 Initiator Model

A virtual `Initiator` represents a PCIe device that originates request TLPs, inferred from the distinct `requester_id` values observed in the trace. Each `Initiator` carries:

- `device_id` — The `requester_id` string, formatted as Bus:Device.Function.
- `total_transactions` — The total number of transactions originated by this device.
- `completed_transactions` — The count of transactions that reached `COMPLETE` state.
- `error_transactions` — The count of transactions that reached `ERROR` state.
- `incomplete_transactions` — The count of transactions that reached `INCOMPLETE` or `TIMEOUT` state.

### 12.2 Target Model

A virtual `Target` represents a PCIe device that issued completion TLPs, inferred from the distinct `completer_id` values observed in completion packets. Each `Target` carries:

- `device_id` — The `completer_id` string.
- `total_completions` — The total number of completions issued by this device.
- `error_completions` — The count of completions carrying UR or CA status.

### 12.3 Device Duality

A device may appear in both the `Initiators` list and the `Targets` list if it both originates requests and satisfies completions for requests from other devices. The two entries are independent; no field is shared or duplicated between them.

---

## 13. Reporting Extensions

### 13.1 Schema Version Advancement

When Phase 5 is enabled, the report's `schema_version` field is set to `"2.0"`. The Phase 1 report structure is preserved in its entirety. Two new top-level sections are added: `transactions` and `virtual_components`.

### 13.2 Transactions Section Structure

The `transactions` section contains:

**`summary`** — Aggregated statistics including total transaction count, count per state (complete, error, timeout, incomplete), and latency statistics (minimum, maximum, and mean latency in nanoseconds across all completed transactions; null if no timestamps were available).

**`system_violations`** — An ordered array of `SystemViolation` records produced during processing and finalization.

**`list`** — A complete ordered array of `Transaction` objects, one per transaction reconstructed from the trace. The ordering is by `txn_id`, which is assigned in the order transactions are first created.

### 13.3 Virtual Components Section Structure

The `virtual_components` section contains:

**`initiators`** — An array of virtual `Initiator` objects, one per distinct `requester_id` observed.

**`targets`** — An array of virtual `Target` objects, one per distinct `completer_id` observed.

### 13.4 Backward Compatibility

A consumer that was built for schema version `1.0` and checks `schema_version` before processing will correctly identify the version as `2.0` and handle it according to its own compatibility policy. A consumer that ignores unknown sections will process all Phase 1 fields correctly and silently skip the new `transactions` and `virtual_components` sections.

---

## 14. GUI Extensions

When Phase 2 is present and Phase 5 is enabled, the following additional views become available. All views are read-only consumers of the report's `transactions` and `virtual_components` sections. No transaction logic resides in the GUI.

### 14.1 Transaction Timeline View

A horizontal timeline visualization displaying all reconstructed transactions on a shared time axis. Each transaction is rendered as a horizontal bar extending from `start_time_ns` to `end_time_ns`. Bar color encodes the terminal state: green for `COMPLETE`, red for `ERROR`, amber for `TIMEOUT`, and gray for `INCOMPLETE`. Hovering over a bar reveals the complete transaction detail. Clicking a bar selects the originating request packet in the main packet table.

### 14.2 Virtual Component Panel

A panel listing all inferred `Initiator` and `Target` components with their aggregate transaction counts and error rates. Clicking a component filters the transaction timeline to display only transactions involving that component.

### 14.3 Latency Distribution Chart

A bar chart presenting the distribution of transaction latencies grouped into configurable buckets. Separate charts are rendered per transaction type. The mean, minimum, and maximum latency values are shown as overlay annotations.

---

## 15. Testing Strategy

### 15.1 Unit Tests

| Component | Required Test Coverage |
|---|---|
| `TransactionMapper.process` | MRd creates a PENDING transaction; CplD transitions it to COMPLETE; a second CplD triggers SYS-003; MWr transitions to COMPLETE immediately when no Cpl is expected |
| `TransactionMapper.finalize` | PENDING transactions present at finalize transition to INCOMPLETE; timeout threshold is correctly evaluated against logical timestamps |
| `VirtualComponentRegistry` | New Initiator registered on first MRd from unseen requester_id; new Target registered on first CplD from unseen completer_id; per-component counters accumulate correctly |
| Latency computation | Correct result with non-zero timestamps; null result when timestamps are zero; negative delta (clock anomaly in trace) flagged correctly |

### 15.2 System Tests

| Test | Expected Outcome |
|---|---|
| Canonical valid trace | All transactions in COMPLETE state; zero system violations; latency values consistent with known timestamps |
| Trace with missing completion | Corresponding transaction transitions to INCOMPLETE at finalize; SYS-002 emitted; SYS-001 emitted if beyond timeout |
| Trace with UR completion | Transaction transitions to ERROR; `completion_status` field is `"UR"`; SYS-004 emitted |
| Phase 1 backward compatibility | Report from a Phase 5 run is parseable by a Phase 1-only consumer; Phase 1 fields are unaltered; `transactions` section is ignored without error |

---

## 16. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation Strategy |
|---|---|---|---|
| Transaction map grows unbounded on traces with many unanswered requests | Medium | Medium | Cap the pending transaction count at a configurable maximum. When the cap is reached, emit SYS-001 and evict the oldest pending transaction to prevent unbounded memory growth. |
| Exception in TransactionMapper prevents Phase 1 report from being written | Low | High | Wrap the entire `TransactionMapper` execution in a try/catch block. Phase 1 `ReportBuilder.write()` is always called in a finally-equivalent block regardless of transaction modeling outcome. |
| Schema v2.0 report breaks Phase 2 GUI report parser | Medium | Medium | Phase 2 `ReportParser` is updated to check `schema_version` and handle both `"1.0"` and `"2.0"` before Phase 5 ships. |
| Timeout threshold misconfiguration produces excessive SYS-001 violations on slow-responding hardware | Medium | Low | Document the default threshold and its implications. The `--txn-timeout-ns 0` option disables timeout detection entirely for environments where latency bounds are not applicable. |

---

## 17. Exit Criteria

Phase 5 is considered complete and accepted when all of the following conditions are simultaneously satisfied:

- [ ] The `TransactionMapper` correctly reconstructs all MRd → CplD transactions in the canonical valid trace, assigning correct states and latency values.
- [ ] All five system-level rules (SYS-001 through SYS-005) are triggered correctly and individually verified by dedicated test traces.
- [ ] Transaction latency is computed correctly for traces with known timestamps, and is null for traces without timestamps.
- [ ] The schema v2.0 report extension is schema-valid and loadable by the Phase 2 GUI without errors.
- [ ] Phase 1 report content is byte-identical to a Phase 1-only run when Phase 5 is disabled.
- [ ] The GUI timeline view correctly renders all transaction states and correctly selects the originating packet on click.
- [ ] All unit tests and system tests pass with zero failures.
- [ ] A `TransactionMapper` exception does not prevent the Phase 1 report from being written.
