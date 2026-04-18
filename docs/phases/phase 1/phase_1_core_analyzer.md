# Phase 1 — Core PCIe Protocol Analyzer

**Document ID:** PCIE-SPEC-PHASE-01  
**Project:** Traceon  
**Version:** 2.0.0  
**Status:** Approved  
**Phase Dependencies:** None — this is the foundational phase of the Traceon platform.  
**Consumed By:** All subsequent phases depend on the components, interfaces, data structures, and report schema defined here.

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Glossary](#2-glossary)
3. [Phase Independence Guarantee](#3-phase-independence-guarantee)
4. [Functional Requirements](#4-functional-requirements)
5. [Non-Functional Requirements](#5-non-functional-requirements)
6. [Architectural Design](#6-architectural-design)
7. [Component Specifications](#7-component-specifications)
8. [Data Model](#8-data-model)
9. [Error Model](#9-error-model)
10. [Validation Rule Registry](#10-validation-rule-registry)
11. [Input Format Specification](#11-input-format-specification)
12. [Output Report Specification](#12-output-report-specification)
13. [Command-Line Interface](#13-command-line-interface)
14. [Testing Strategy](#14-testing-strategy)
15. [Risks and Mitigations](#15-risks-and-mitigations)
16. [Exit Criteria](#16-exit-criteria)

---

## 1. Purpose and Scope

### 1.1 Purpose

Phase 1 delivers the **Core PCIe Protocol Analyzer** — the analytical heart of the Traceon platform. It is a deterministic, offline, headless engine that ingests PCIe trace data from the filesystem, decodes Transaction Layer Packets at the structural level, enforces protocol correctness through a rule-based validation engine, and produces structured machine-readable analysis reports. This phase is a complete and independently shippable product. Every subsequent phase in Traceon is built on top of the capabilities and contracts established here, and no later phase is permitted to alter what this phase defines.

### 1.2 Objectives

The following objectives govern the scope and success of Phase 1:

- Deliver reliable offline PCIe trace analysis on Linux with no external runtime dependencies beyond the C++17 standard library.
- Decode raw hexadecimal trace packets into richly structured TLP representations, with full extraction of every defined header field.
- Enforce PCIe transaction-level protocol correctness through a deterministic, registerable, rule-based validation engine.
- Emit analysis reports in structured JSON or XML format that are suitable for consumption by engineers, automated tools, and all downstream Traceon phases.
- Establish the permanent authoritative analysis core of the Traceon platform — an engine whose behavior, interfaces, and outputs are fixed and may only be extended, never overwritten, by later phases.

### 1.3 In-Scope Capabilities

| Capability | Description |
|---|---|
| Offline trace ingestion | Sequential reading and parsing of PCIe trace files from the local filesystem |
| TLP structural decoding | Conversion of raw hexadecimal packet payloads into typed TLP objects with all header fields extracted |
| Protocol validation | Stateful, rule-based detection of transaction-level protocol violations |
| Error classification | Strict separation of structural decode errors from semantic protocol violations |
| Report generation | Production of a complete, schema-versioned analysis report in JSON or XML |
| CLI execution | Fully headless operation driven entirely by command-line arguments |
| Deterministic behavior | Identical inputs always produce identical outputs, with no dependence on time, environment, or execution order |
| Unit and system testing | Validation of correctness for all components against a defined set of known-good and known-bad trace inputs |

### 1.4 Explicitly Out of Scope

The following capabilities are intentionally absent from Phase 1. Their absence is a deliberate design constraint, not an oversight. Each is addressed in a named subsequent phase.

| Excluded Capability | Rationale and Destination Phase |
|---|---|
| Graphical user interface | Addressed in Phase 2; must not exert any influence on the core engine design |
| Multithreading or concurrent processing | Addressed in Phase 4; single-threaded correctness must be fully established and verified first |
| Live or streaming packet ingestion | Requires the concurrent infrastructure introduced in Phase 4 |
| Python or Tcl scripting interface | Addressed in Phase 3 |
| SystemC or Transaction-Level Modeling | Addressed in Phase 5 |
| Controlled fault injection | Addressed in Phase 6 |
| Multi-protocol or cross-domain analysis | Addressed in Phase 7 |

---

## 2. Glossary

| Term | Definition |
|---|---|
| **TLP** | Transaction Layer Packet. The fundamental unit of PCIe communication at the transaction layer. All higher-level operations are expressed as one or more TLPs. |
| **RawPacket** | A timestamped, directional container of uninterpreted hexadecimal bytes, representing a single line from a trace file before any protocol-aware processing. |
| **DW** | Double Word. A 32-bit (4-byte) unit. PCIe header sizes and payload lengths are expressed in DWs. |
| **DW0 / DW1 / DW2 / DW3** | The first through fourth 32-bit words of a TLP header, indexed from zero. |
| **Fmt** | The Format field, occupying bits 31 through 29 of DW0. Encodes the header size (3DW or 4DW) and whether a data payload is attached. |
| **Type** | The packet Type field, occupying bits 28 through 24 of DW0. In combination with Fmt, uniquely identifies the TLP type. |
| **MRd** | Memory Read Request. A TLP issued by a requester to read data from a memory address. Requires a subsequent CplD to close the transaction. |
| **MWr** | Memory Write Request. A TLP issued by a requester to write data to a memory address. May optionally be acknowledged by a Cpl. |
| **Cpl** | Completion without Data. Acknowledges a write or signals an error response to a prior request. |
| **CplD** | Completion with Data. Returns the requested data in response to an MRd. Closes the outstanding request when received. |
| **Requester ID** | A 16-bit PCIe Bus:Device.Function identifier that uniquely identifies the device that originated a request TLP. |
| **Completer ID** | A 16-bit PCIe Bus:Device.Function identifier that uniquely identifies the device that issued a completion TLP. |
| **Tag** | An 8-bit field embedded in request TLPs, assigned by the requester. Used to match an MRd to its corresponding CplD. |
| **TC** | Traffic Class. A 3-bit Quality-of-Service priority field with a valid range of 0 through 7. |
| **AT** | Address Type. A 2-bit field in MRd and MWr TLPs indicating untranslated, translation request, or translated address types. |
| **SC** | Successful Completion. The completion status code value `000b`, indicating that the request was satisfied normally. |
| **UR** | Unsupported Request. The completion status code value `001b`, indicating that the target does not support the requested operation. |
| **CA** | Completer Abort. The completion status code value `100b`, indicating that the completer was unable to complete the request. |
| **Decode Error** | A structural defect detected while parsing a RawPacket into a TLP. Indicates that the raw bytes do not conform to the PCIe packet structure. |
| **Validation Error** | A protocol rule violation detected in a structurally valid TLP. Indicates that the packet is well-formed but its semantics violate a PCIe transaction rule. |
| **is_malformed** | A Boolean field on a TLP object. When true, the packet carries one or more decode errors and its field data is partially or completely unreliable. |
| **Outstanding Request** | An MRd TLP for which no corresponding CplD has yet been observed during processing of the trace. |
| **TransactionKey** | The composite key `(requester_id, tag)` used to uniquely identify an outstanding request and match it to its completion. |

---

## 3. Phase Independence Guarantee

Phase 1 is self-contained and must remain permanently operational regardless of which subsequent phases are present, configured, or enabled. The following constraints are binding on all contributors across the entire Traceon project lifecycle:

**Rule 1 — No modification of Phase 1 source files.** After Phase 1 acceptance, its source files are read-only. Future phases extend functionality by wrapping or consuming Phase 1 interfaces. No future phase may alter Phase 1 behavior by patching its internals.

**Rule 2 — Core components remain protocol-pure.** The `PacketDecoder` and `ProtocolValidator` components must remain permanently unaware of any networking layer, graphical interface, scripting environment, or hardware abstraction. Their only inputs are the data structures defined in this document. Injecting any cross-cutting concern into these components is a design violation.

**Rule 3 — Independent buildability.** Phase 1 must be compilable and executable in complete isolation. No build flag, conditional compilation directive, or configuration option may be required to run Phase 1 independently of later phases.

**Rule 4 — Versioned report schema.** Phase 1 establishes version `1.0` of the Traceon analysis report schema. Any future phase that extends the report schema must increment the version identifier and maintain full backward compatibility for all fields defined in version `1.0`. A Phase 1-only consumer must be able to safely parse a version `1.0` report produced by any version of the Traceon platform.

---

## 4. Functional Requirements

### 4.1 Trace Ingestion

| ID | Requirement |
|---|---|
| FR-ING-01 | The system shall accept a trace file path as a mandatory command-line argument at startup. |
| FR-ING-02 | The system shall read packets from the trace file strictly sequentially, preserving the original order of all packets as they appear in the file. |
| FR-ING-03 | Each parsed trace line shall be represented as a `RawPacket` containing the fields `index`, `timestamp_ns`, `direction`, and `payload_hex`. |
| FR-ING-04 | A zero-based, monotonically increasing integer index shall be assigned to each successfully parsed packet, regardless of the presence of timestamps or other metadata. |
| FR-ING-05 | When a timestamp is absent from a trace line, the `timestamp_ns` field shall be set to zero. The absence of a timestamp is not a parse error. |
| FR-ING-06 | A trace line that cannot be parsed due to structural malformation — such as an incorrect number of delimited fields — shall be logged as an ingestion warning, skipped entirely, and counted in a running skipped-line counter. Processing of subsequent lines shall continue without interruption. |
| FR-ING-07 | The system shall not terminate abnormally for any trace file input whatsoever, including completely empty files, files containing only malformed lines, files with no trailing newline, and files containing comment lines. |

### 4.2 Packet Decoding

| ID | Requirement |
|---|---|
| FR-DEC-01 | The system shall decode each `RawPacket` into a `TLP` by sequentially interpreting the hexadecimal payload bytes as the DW0, DW1, DW2, and optionally DW3 header words. |
| FR-DEC-02 | The system shall extract all named fields defined in Section 8.2 from each TLP, assigning typed values to each field based on the bit positions specified in the PCIe Base Specification. |
| FR-DEC-03 | The system shall support decoding of the following TLP types: Memory Read Request (MRd), Memory Write Request (MWr), Completion without Data (Cpl), and Completion with Data (CplD). |
| FR-DEC-04 | The system shall detect all structural error conditions enumerated in Section 9.1 and record each detected error as a `DecodeError` entry within the `TLP.decode_errors` collection. |
| FR-DEC-05 | When at least one structural error is detected during the decoding of a packet, the `TLP.is_malformed` flag shall be set to `true`. |
| FR-DEC-06 | The decoder shall produce a `TLP` object for every `RawPacket` processed, including those that are structurally malformed. Malformed TLPs carry whatever partial field data could be extracted up to the point at which the error was encountered. |
| FR-DEC-07 | Decoding shall be fully deterministic. Identical input bytes presented in identical order shall always produce an identical output TLP, including identical `decode_errors` entries, with no variation across runs, environments, or hardware. |
| FR-DEC-08 | The decoder shall carry no internal state between packets. Each packet is decoded as a completely independent operation. The result of decoding any one packet must not depend on any previously decoded packet. |

### 4.3 Protocol Validation

| ID | Requirement |
|---|---|
| FR-VAL-01 | Only non-malformed TLPs shall be passed to the validation engine. TLPs with `is_malformed` set to `true` are excluded from protocol validation entirely. |
| FR-VAL-02 | The system shall detect all protocol violations defined in the Validation Rule Registry in Section 10. |
| FR-VAL-03 | The validation engine shall maintain an outstanding-request table, keyed by the composite `(requester_id, tag)` pair, throughout the processing of a trace. |
| FR-VAL-04 | When an MRd TLP is observed, its `(requester_id, tag)` pair shall be inserted into the outstanding-request table along with its packet index, length, and timestamp. |
| FR-VAL-05 | When a CplD or Cpl TLP is observed, the engine shall attempt to locate a matching entry in the outstanding-request table using the `(requester_id, tag)` key. If no matching entry exists, a duplicate-completion violation shall be recorded. |
| FR-VAL-06 | When a valid completion is matched to an outstanding request, the corresponding entry shall be removed from the outstanding-request table, closing the transaction. |
| FR-VAL-07 | Upon completion of trace processing, all entries that remain in the outstanding-request table shall be reported as missing-completion violations, one record per unmatched request. |
| FR-VAL-08 | Validation shall be fully deterministic. Identical input sequences shall always produce identical violation sets with no variation. |

### 4.4 Reporting

| ID | Requirement |
|---|---|
| FR-RPT-01 | The system shall produce a single, complete analysis report upon successful conclusion of trace processing. |
| FR-RPT-02 | The report shall contain a summary section enumerating the total packet count, the distribution of packets by TLP type, the total count of decode errors, and the total count of validation errors. |
| FR-RPT-03 | The report shall contain a complete, ordered list of all detected decode errors, each record carrying the `packet_index`, `rule_id`, the affected `field` name, and a human-readable `description`. |
| FR-RPT-04 | The report shall contain a complete, ordered list of all detected validation errors, each record carrying the `rule_id`, `category`, `packet_index`, `related_index` where applicable, and a human-readable `description`. |
| FR-RPT-05 | The report shall be written to the output path supplied via the `--output` command-line argument. |
| FR-RPT-06 | The default report format shall be JSON. XML output shall be selectable via the `--format xml` argument. |
| FR-RPT-07 | The report shall include a `schema_version` field set to the string value `"1.0"`, enabling consumers to perform format compatibility verification before processing. |

---

## 5. Non-Functional Requirements

| ID | Category | Requirement |
|---|---|---|
| NFR-01 | Reliability | The analyzer shall not terminate abnormally under any input condition. Every error, whether structural or semantic, shall be caught, classified, and reported. The system shall always produce a report. |
| NFR-02 | Reliability | A complete and well-formed report shall always be produced upon completion of processing, even when the trace contains exclusively malformed packets or exclusively protocol violations. |
| NFR-03 | Performance | The analyzer shall process a trace file containing one hundred thousand packets in under ten seconds when executed on a single-core 1 GHz reference system with a warm filesystem cache. |
| NFR-04 | Memory | Peak memory consumption shall not exceed 512 megabytes for any trace file containing up to one million packets. Streaming ingestion shall be employed to avoid loading the entire trace into memory simultaneously. |
| NFR-05 | Portability | The analyzer shall compile without modification and execute correctly on Linux x86-64 with kernel version 4.15 or later. |
| NFR-06 | Portability | No platform-specific system calls, compiler extensions, or operating-system-specific APIs shall appear in the codebase. The implementation shall be strictly conformant C++17. |
| NFR-07 | Maintainability | Each component shall have a single, clearly stated responsibility that does not overlap with any other component. No component shall contain logic whose natural home is another layer. |
| NFR-08 | Extensibility | Adding a new validation rule shall require changes exclusively within the Validation Layer and the rule registry. No component outside those boundaries shall require modification when a new rule is introduced. |
| NFR-09 | Testability | Every validation rule shall have at minimum one unit test covering the positive case (no violation produced from conformant input) and one unit test covering the negative case (violation correctly produced from non-conformant input). |
| NFR-10 | Correctness | The analyzer shall produce zero false-positive decode errors and zero false-positive validation errors when processing the canonical valid trace file distributed with the Traceon platform. |

---

## 6. Architectural Design

### 6.1 Layered Architecture

The Phase 1 system is organized into four strictly isolated, vertically stacked processing layers. Data flows in one direction only — from the top layer downward. No layer has any visibility into the internal implementation details of any layer other than the one immediately above it, and all inter-layer communication occurs exclusively through the data structures defined in Section 8.

```
┌──────────────────────────────────────────────────────┐
│                 Trace Input Layer                    │
│  Reads files, constructs and emits RawPacket objects │
├──────────────────────────────────────────────────────┤
│               Packet Decode Layer                    │
│  Transforms each RawPacket into a typed TLP object   │
├──────────────────────────────────────────────────────┤
│             Protocol Validation Layer                │
│  Applies stateful transaction rules over TLP stream  │
├──────────────────────────────────────────────────────┤
│                 Reporting Layer                      │
│  Aggregates all results and serializes the report    │
└──────────────────────────────────────────────────────┘
```

### 6.2 Inter-Layer Communication Contracts

Each layer communicates with the next exclusively through a defined data structure. Passing raw bytes, internal implementation types, or opaque handles across a layer boundary is strictly forbidden.

| Producer Layer | Consumer Layer | Data Contract |
|---|---|---|
| Trace Input | Packet Decode | `RawPacket` — one per ingested trace line |
| Packet Decode | Protocol Validation | `TLP` — non-malformed only; one per decoded RawPacket |
| Packet Decode | Reporting | `TLP` — all, including malformed |
| Protocol Validation | Reporting | `ValidationError[]` — zero or more per processed TLP, plus finalization results |

### 6.3 Architectural Rationale

**Layer isolation.** Each layer addresses a fundamentally distinct concern: file I/O and parsing, binary structure interpretation, semantic protocol reasoning, and output formatting. Mixing any two of these concerns into a single component creates a component that is harder to test in isolation, harder to reason about, and harder to extend. The strict isolation established here is what makes Phase 4's introduction of concurrent queuing possible without touching the decoder or validator — they never knew where their input came from.

**Stateless decoding.** The decoder carries no state between packets. This is both a correctness constraint — protocol rules belong in the validator, not the decoder — and a forward-compatibility constraint. A stateless decoder can be invoked from any thread at any time without synchronization overhead, which is a prerequisite for the parallel processing model introduced in Phase 4.

**Malformed packet filtering at validation boundary.** Protocol rules as defined by the PCIe Base Specification assume structurally valid packets. Applying transaction-matching logic to a packet with an unreadable tag field, a corrupted requester ID, or an illegal Fmt value would produce results that are meaningless at best and actively misleading at worst. Strict separation of the two populations ensures that every decision made by the validator is grounded in reliable data.

### 6.4 Orchestrator Role

A top-level `AnalyzerEngine` component connects the four layers and manages the processing loop. It is responsible for constructing each component in the correct order, driving the per-packet iteration, routing each decoded TLP to the appropriate downstream consumers, invoking the validator's finalization step after all packets are processed, and writing the report to the output path. The `AnalyzerEngine` contains no protocol logic and no parsing logic. It is a pure coordinator.

---

## 7. Component Specifications

### 7.1 TraceInputLayer

**Single Responsibility:** Read a trace file from the filesystem, parse each text line into a `RawPacket`, and present packets to callers one at a time through a sequential iterator-style interface.

**Behavioral Contract:**
- Accepts a filesystem path at construction time and opens the file at that point. If the file cannot be opened, the component shall raise a typed `TraceInputException` rather than proceeding with a null handle.
- Provides a `next()` operation that returns the next available `RawPacket`, or a sentinel indicating exhaustion when the file is fully consumed.
- Provides an `is_exhausted()` query that returns true after all lines have been consumed.
- Exposes a `skipped_line_count()` accessor reporting the number of lines dropped due to parse failures.
- Shall read the file line-by-line, one line per call to `next()`. The entire file must never be loaded into memory simultaneously.
- Has no knowledge of PCIe protocol structure. It does not inspect, interpret, or validate `payload_hex` bytes in any way.
- Shall not raise an exception for a malformed line. Instead, it logs the warning, increments the skip counter, and proceeds to the next line.

### 7.2 PacketDecoder

**Single Responsibility:** Accept a single `RawPacket` and produce a corresponding `TLP` by extracting all defined header fields from the hexadecimal payload and recording any structural defects encountered during extraction.

**Behavioral Contract:**
- Accepts a `RawPacket` as input and returns a `TLP` as output, unconditionally. There is no input for which this operation fails to return a result.
- If decoding succeeds completely, the returned TLP has `is_malformed` set to `false` and `decode_errors` is empty.
- If a structural defect is encountered at any point during decoding, the error is recorded in `decode_errors`, `is_malformed` is set to `true`, and whatever field data was successfully extracted before the point of failure is preserved in the TLP.
- Shall never throw an exception. All error conditions are communicated through `decode_errors`.
- Carries no internal state whatsoever. The component may be called with packets in any order, any number of times, on any thread, without coordination, and will always produce the same output for the same input.
- Never reads from or writes to any global or shared mutable state.

### 7.3 ProtocolValidator

**Single Responsibility:** Accept a stream of structurally valid TLPs one at a time, maintain the outstanding-request table that tracks all unresolved MRd transactions, evaluate all registered protocol rules against each incoming TLP, and report any violations detected either in-stream or at end-of-trace finalization.

**Behavioral Contract:**
- Provides a `process(TLP)` operation that evaluates the TLP against all applicable protocol rules and updates the outstanding-request table accordingly.
- Callers must never pass a TLP with `is_malformed == true` to this component. The behavior for such an input is undefined.
- Provides a `finalize()` operation that must be called exactly once, after all calls to `process()` are complete. `finalize()` scans the outstanding-request table for any unmatched requests and emits missing-completion violations for each one. It must not be called more than once.
- Provides an `errors()` accessor returning the accumulated `ValidationError` collection at any point during or after processing.
- Internal state consists of the outstanding-request table, keyed by `(requester_id, tag)` pairs, and the accumulated error collection. Both are private and inaccessible to callers.

### 7.4 ReportBuilder

**Single Responsibility:** Accept the full set of decoded TLPs and validation errors produced by upstream components, compute aggregated summary statistics, and serialize the complete analysis report to the output format selected at construction time.

**Behavioral Contract:**
- Accepts each TLP via `add_tlp()`, including malformed ones. The distinction between malformed and valid TLPs informs the summary statistics.
- Accepts each validation error via `add_validation_error()`.
- Provides a `write(output_path)` operation that serializes the complete report to the specified path. Raises a typed `ReportWriteException` if the path cannot be written.
- Contains no protocol logic. It does not re-evaluate rules, re-derive errors, or make any interpretation of TLP semantics. It formats what it has been given.
- Must produce a report that is schema-conformant with respect to the field structure defined in Section 12.

### 7.5 AnalyzerEngine

**Single Responsibility:** Wire the four processing layers together, drive the processing loop, and manage top-level execution lifecycle including startup validation, processing, and graceful shutdown.

**Processing Loop Specification:**

The engine constructs a `TraceInputLayer`, `PacketDecoder`, `ProtocolValidator`, and `ReportBuilder` in that order, then enters the processing loop. On each iteration, it calls `TraceInputLayer.next()` to obtain the next `RawPacket`. If the layer signals exhaustion, the loop terminates. Otherwise, the packet is passed to `PacketDecoder.decode()`, and the resulting TLP is registered with `ReportBuilder.add_tlp()`. If the TLP is not malformed, it is additionally passed to `ProtocolValidator.process()`. After the loop terminates, `ProtocolValidator.finalize()` is called and the resulting validation errors are each registered with `ReportBuilder.add_validation_error()`. Finally, `ReportBuilder.write()` is invoked to produce the output file.

---

## 8. Data Model

### 8.1 RawPacket

`RawPacket` is the data contract between the Trace Input Layer and the Packet Decode Layer. It represents a single trace entry prior to any protocol-aware interpretation.

| Field | Type | Description |
|---|---|---|
| `index` | `uint64_t` | Zero-based sequential packet number assigned by the ingestion layer |
| `timestamp_ns` | `uint64_t` | Capture timestamp in nanoseconds; zero if not present in the trace |
| `direction` | `string` | Packet direction indicator: `"TX"` or `"RX"` |
| `payload_hex` | `string` | Raw packet bytes encoded as a contiguous hexadecimal string |

### 8.2 TLP Field Definitions

The TLP object is the central data structure of the Phase 1 engine. All fields are populated by the `PacketDecoder` from the raw byte payload.

| Field | Source | Type | Description |
|---|---|---|---|
| `type` | DW0[28:24] + DW0[31:29] | `TlpType` | Enumerated TLP type: MRd, MWr, Cpl, CplD |
| `header_fmt` | DW0[31:29] | `string` | Header format indicator: `"3DW"` or `"4DW"` |
| `has_data` | DW0[30] | `bool` | True when the Fmt field indicates a data payload is present |
| `tc` | DW0[22:20] | `uint8_t` | Traffic Class value (0–7) |
| `length_dw` | DW0[9:0] | `uint16_t` | Payload length in Double Words |
| `attr.no_snoop` | DW0[12] | `bool` | No Snoop attribute bit |
| `attr.relaxed_ordering` | DW0[13] | `bool` | Relaxed Ordering attribute bit |
| `at` | DW0[9:8] (MRd/MWr) | `uint8_t` | Address Type field |
| `requester_id` | DW1[31:16] | `string` | Bus:Device.Function string of the originating device |
| `tag` | DW1[15:8] | `uint8_t` | Transaction tag for request/completion matching |
| `last_be` | DW1[7:4] | `uint8_t` | Last DW Byte Enable |
| `first_be` | DW1[3:0] | `uint8_t` | First DW Byte Enable |
| `address` | DW2[31:2] (3DW) or DW2+DW3 (4DW) | `string` | Target memory address in hexadecimal notation |
| `completer_id` | DW1[31:16] (Cpl/CplD) | `string` | Bus:Device.Function of the completing device |
| `status` | DW1[15:13] (Cpl/CplD) | `string` | Completion status: `"SC"`, `"UR"`, or `"CA"` |
| `byte_count` | DW1[11:0] (Cpl/CplD) | `uint16_t` | Remaining byte count for the completion |
| `is_malformed` | Derived | `bool` | True when at least one decode error was recorded |
| `decode_errors` | Derived | `DecodeError[]` | All structural errors found during decoding |

Fields that are structurally inapplicable to a given TLP type — such as `address` on a completion, or `byte_count` on a request — shall be represented as `null` in the report output. Inapplicable fields are never omitted; they are always present and always null.

### 8.3 DecodeError

| Field | Type | Description |
|---|---|---|
| `rule_id` | `string` | The DEC-xxx rule identifier from Section 9.1 |
| `field` | `string` | The name of the TLP header field in which the error was detected |
| `description` | `string` | Human-readable explanation of the specific structural violation |

### 8.4 ValidationError

| Field | Type | Description |
|---|---|---|
| `rule_id` | `string` | The VAL-xxx rule identifier from Section 10 |
| `category` | `string` | Symbolic category name matching the rule definition |
| `packet_index` | `uint64_t` | Index of the primary packet involved in the violation |
| `related_index` | `uint64_t` or `null` | Index of the secondary packet where applicable; null otherwise |
| `description` | `string` | Human-readable explanation of the specific protocol violation |

---

## 9. Error Model

### 9.1 Structural Decode Error Rules

The following decode error rules are evaluated by the `PacketDecoder` during the processing of each RawPacket. A rule triggers whenever the designated structural invariant is violated. Multiple rules may trigger for the same packet.

For the complete registry of Phase 1 structural decode rules, including Rule IDs, triggering conditions, and field mappings, refer to the [Structural Decode Error Registry](decoding_error_rules.md).

### 9.2 Error Classification Policy

Decode errors and validation errors are categorically distinct and are never conflated:

- A **decode error** is a structural property of a single packet in isolation. It is detected before any protocol context is considered. A packet with a decode error is malformed; its field values are unreliable.
- A **validation error** is a semantic property of a packet in the context of other packets. It is detected by evaluating protocol rules over a stream of structurally valid packets. A packet that triggers a validation error is structurally well-formed; it simply violates a behavioral rule.

No TLP may be both malformed and the subject of a validation error. The validation engine never processes malformed TLPs.

---

## 10. Validation Rule Registry

The validation rules evaluated by the `ProtocolValidator` are maintained in a separate registry to facilitate extension and prevent documentation bloat.

For the complete registry of Phase 1 validation rules, including Rule IDs, triggering conditions, and categories, refer to the [Validation Rule Registry](validation_rule_registry.md).

---

## 11. Input Format Specification

### 11.1 Trace File Format

Trace files are plain text, with one packet per line. Lines are parsed as comma-separated values. Lines beginning with the `#` character are treated as comments and silently skipped. Blank lines are silently skipped. The ordering of packets in the file is the authoritative ordering for analysis; no sorting or reordering is performed by the ingestion layer.

### 11.2 Required Line Fields

| Position | Field Name | Format | Required |
|---|---|---|---|
| 1 | `timestamp_ns` | Unsigned decimal integer | No — zero if absent or blank |
| 2 | `direction` | String literal `TX` or `RX` | Yes |
| 3 | `payload_hex` | Contiguous hexadecimal string, no spaces | Yes |

A line with fewer than two fields after splitting on commas shall be classified as malformed, skipped, and counted toward the skipped-line counter.

### 11.3 Comment and Empty Line Handling

Lines beginning with `#` are unconditionally ignored and do not contribute to any packet count or error count. Empty lines — lines containing only whitespace — are similarly ignored. Neither type of ignored line increments the skipped-line counter; the counter records only structurally malformed data lines.

---

## 12. Output Report Specification

### 12.1 Top-Level Report Structure

The analysis report is a hierarchically structured document containing the following top-level sections:

**`schema_version`** — The string value `"1.0"`. Consumers must check this field before processing any other part of the report.

**`generated_at`** — An ISO 8601 UTC timestamp recording when the report was produced.

**`trace_file`** — The absolute or relative path of the trace file that was analyzed.

**`summary`** — An aggregated statistics object containing the total packet count, per-type TLP distribution map, total malformed packet count, total validation error count, and skipped line count.

**`malformed_packets`** — An ordered array of objects, one per packet that triggered at least one decode error. Each object carries the `packet_index`, `timestamp_ns`, `direction`, `payload_hex`, and a `decode_errors` array detailing each specific structural violation.

**`validation_errors`** — An ordered array of `ValidationError` records, one per detected protocol violation. Ordering reflects the order in which violations were produced during processing.

**`packets`** — An ordered array containing one record per ingested packet in original trace order. Each record contains the packet's `index`, `timestamp_ns`, `direction`, `is_malformed` flag, and a `tlp` sub-object with all extracted header fields.

### 12.2 Field Null Serialization Policy

Fields that are structurally inapplicable for a given TLP type must appear in the output as JSON `null` or the XML equivalent. Inapplicable fields must never be omitted from the output. This policy ensures that the report schema is uniform across all TLP types and that consumers can always rely on the presence of every field name.

### 12.3 Schema Version Contract

The `schema_version` field governs backward compatibility. Changes to the report structure that remove, rename, or alter the semantics of any existing field constitute a breaking change and must increment the schema version. Additive changes — new optional fields, new array entries — may be introduced without incrementing the version, provided that existing consumers that ignore unknown fields will not be broken.

---

## 13. Command-Line Interface

### 13.1 Invocation Syntax

```
pcie_analyzer --input <trace_file> --output <report_file> [options]
```

### 13.2 Arguments

| Argument | Required | Default | Description |
|---|---|---|---|
| `--input <path>` | Yes | — | Filesystem path to the input trace file. |
| `--output <path>` | Yes | — | Filesystem path for the generated output report. |
| `--format <json\|xml>` | No | `json` | Format of the output report. |
| `--verbose` | No | Off | Print per-packet decode and validation progress to standard output during processing. |
| `--version` | No | — | Print the Traceon analyzer version string and exit without processing. |
| `--help` | No | — | Print a complete usage reference and exit without processing. |

### 13.3 Exit Codes

| Code | Meaning |
|---|---|
| `0` | Analysis completed successfully. Report was written to the output path. |
| `1` | The input trace file could not be opened or read. |
| `2` | The output path could not be written. The report was not persisted. |
| `3` | One or more command-line arguments were invalid or incompatible. |

**Design note on exit codes:** The presence of decode errors or validation errors in the analysis results does not produce a non-zero exit code. The analyzer's responsibility is to analyze and report; the interpretation of findings is the responsibility of the caller. Exit codes 1 through 3 are reserved for execution-level failures that prevent the analyzer from fulfilling its basic function.

---

## 14. Testing Strategy

### 14.1 Unit Testing

Each component shall have a dedicated unit test suite. Unit tests shall operate exclusively on the component under test, using no file I/O and no inter-component dependencies.

| Component | Required Test Coverage |
|---|---|
| `PacketDecoder` | One test per decode error rule (DEC-001 through DEC-009) verifying the rule triggers correctly. One test per supported TLP type verifying fully successful decoding with correct field extraction. Additional tests for boundary conditions on each named field. |
| `ProtocolValidator` | One test per validation rule (VAL-001 through VAL-008) verifying both the conformant case and the violating case. Multi-packet sequence tests for tag insertion, matching, and removal. Finalization tests verifying outstanding request detection. |
| `TraceInputLayer` | Tests for: empty file, comment-only file, mixed valid and malformed lines, file with no trailing newline, file with only blank lines. Verification that skipped-line count is accurate. |
| `ReportBuilder` | JSON output correctness test. XML output correctness test. Null field serialization test. Empty report test (zero packets, zero errors). Schema version field verification. |

### 14.2 System Testing

System tests exercise the complete processing pipeline from trace file to output report, validating end-to-end correctness.

| Test Scenario | Expected Outcome |
|---|---|
| Canonical valid trace | Report produced with zero decode errors and zero validation errors. Summary statistics match packet composition of input. |
| Trace with one instance of each malformed packet type | Each decode error rule DEC-001 through DEC-009 triggered exactly once. Zero validation errors. |
| Trace with one instance of each validation violation type | Zero decode errors. Each validation rule VAL-001 through VAL-008 triggered exactly once. |
| Empty trace file | Report produced with zero packets, zero errors, and zero skipped lines. |
| Large trace (100,000 packets or more) | Processing completes within the bound established by NFR-03. |
| Comment-and-blank-line-only trace | Report produced with zero packets. Skipped-line count reflects only malformed data lines, not comment or blank lines. |

### 14.3 Regression Testing

A set of canonical trace files together with their precisely expected report outputs shall be committed to the Traceon repository. After any change to Phase 1 components, the system test suite shall re-execute all canonical trace files and compare the resulting reports to the committed expected outputs byte-for-byte. Any deviation is a regression and must be resolved before the change is accepted.

---

## 15. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation Strategy |
|---|---|---|---|
| Ambiguous interpretation of the PCIe Base Specification produces incorrect field extraction for edge-case packets | Medium | High | Document every bit-level field extraction rule explicitly in the decoder specification. Cross-validate the decoder output against known-good reference implementations and hardware capture tools during the testing phase. |
| Memory growth becomes unbounded for extremely large trace files | Medium | Medium | Enforce streaming ingestion at the TraceInputLayer boundary. The decoded TLP collection in the ReportBuilder shall not retain TLPs after writing them to the output stream; only summary statistics are retained in memory across the full trace. |
| The outstanding-request table becomes a performance bottleneck for traces with very large numbers of simultaneous unresolved requests | Low | Low | Use a hash map with the `(requester_id, tag)` composite key. Profile the validator if the number of simultaneously outstanding requests exceeds ten thousand entries in a production trace. |
| An output file write failure silently discards analysis results | Low | High | Always verify the return status of every file I/O operation in the ReportBuilder. Return exit code 2 and log the failure path to standard error before terminating. Never allow a write failure to propagate silently. |
| Future phases inadvertently break Phase 1 correctness by modifying shared components | Medium | High | After Phase 1 acceptance, Phase 1 source files are immutable. A CI pipeline job shall fail if any Phase 1 source file is modified after the Phase 1 release tag is applied. |

---

## 16. Exit Criteria

Phase 1 is considered complete and accepted for production use when all of the following conditions are simultaneously satisfied:

- [ ] The analyzer processes the canonical valid trace file and produces a report with zero errors and zero abnormal terminations.
- [ ] All nine decode error rules (DEC-001 through DEC-009) are triggered correctly and individually verified by the malformed-packet test suite.
- [ ] All eight validation rules (VAL-001 through VAL-008) are triggered correctly and individually verified by the validation test suite.
- [ ] All unit tests for all four components pass with zero failures and zero unexpected behaviors.
- [ ] All system tests pass with zero failures.
- [ ] NFR-03 (performance) is demonstrated by benchmark measurement on the reference system using the 100,000-packet large-trace test.
- [ ] NFR-04 (memory) is demonstrated by profiler measurement on the large-trace test.
- [ ] The CLI interface exactly matches the specification in Section 13, including all argument names, defaults, and exit codes.
- [ ] The JSON report output exactly matches the schema specified in Section 12, including null field serialization for all inapplicable fields.
- [ ] This specification document and the CLI user guide are complete, reviewed, and approved.

Upon satisfying all exit criteria, Phase 1 constitutes a **complete, standalone, production-ready PCIe analysis tool** — independently usable without any other Traceon phase installed.
