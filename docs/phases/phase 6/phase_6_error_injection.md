# Phase 6 — Error Injection and Fault Modeling

**Document ID:** PCIE-SPEC-PHASE-06  
**Project:** Traceon  
**Version:** 2.0.0  
**Status:** Approved  
**Phase Dependencies:** Phase 1 (PCIE-SPEC-PHASE-01) — fault injection targets the `RawPacket` stream before it reaches the `PacketDecoder`. Phase 4 (PCIE-SPEC-PHASE-04) — in live mode, the injection layer intercepts packets on the producer side of the `QueueManager`. Phases 2, 3, and 5 are optional integrations described in their respective sections.  
**Consumed By:** No subsequent phase depends on Phase 6. It is a verification overlay that operates on top of all prior phases.

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Glossary](#2-glossary)
3. [Phase Independence Guarantee](#3-phase-independence-guarantee)
4. [Fault Modeling Philosophy](#4-fault-modeling-philosophy)
5. [Functional Requirements](#5-functional-requirements)
6. [Non-Functional Requirements](#6-non-functional-requirements)
7. [Architectural Design](#7-architectural-design)
8. [Fault Taxonomy](#8-fault-taxonomy)
9. [Component Specifications](#9-component-specifications)
10. [Data Structures](#10-data-structures)
11. [Fault Configuration Specification](#11-fault-configuration-specification)
12. [Fault Lifecycle](#12-fault-lifecycle)
13. [Diagnostic Reporting](#13-diagnostic-reporting)
14. [GUI Integration](#14-gui-integration)
15. [Testing Strategy](#15-testing-strategy)
16. [Risks and Mitigations](#16-risks-and-mitigations)
17. [Exit Criteria](#17-exit-criteria)

---

## 1. Purpose and Scope

### 1.1 Purpose

Phase 6 introduces **controlled fault injection and fault modeling** to the Traceon platform. Its purpose is to evaluate the robustness of the analysis pipeline under abnormal protocol conditions, to verify the correctness and completeness of Phase 1's error detection logic, and to provide a systematic mechanism for negative-path testing that requires no modification to any previously delivered component. Phase 6 is a verification instrument — it determines whether the platform detects what it should detect.

### 1.2 Objectives

- Provide a configurable `FaultInjectionLayer` that transparently intercepts the `RawPacket` stream and introduces controlled structural and protocol-level faults according to a declarative configuration file.
- Verify that every decode error rule (DEC-001 through DEC-009) and every validation rule (VAL-001 through VAL-008) is triggered correctly when the corresponding fault is injected.
- Ensure that every injected fault is traceable: each fault appears in the diagnostic report alongside the system's detected response and an outcome classification.
- Guarantee that the normal processing path is completely unaffected when fault injection is disabled, producing byte-identical reports to pre-Phase 6 execution.

### 1.3 In-Scope Capabilities

| Capability | Description |
|---|---|
| Packet-level fault injection | Corrupt raw packet bytes before they reach the decoder |
| Transaction-level fault modeling | Suppress, duplicate, or alter completions in the packet stream |
| Timing and ordering faults | Artificially alter timestamps or swap the stream positions of two packets |
| Scenario-based configuration | Define named, reusable fault scenarios in JSON configuration files |
| Fault diagnostic reporting | Produce a per-fault report identifying what was injected and whether the expected rule was triggered |
| Fault coverage analysis | Measure and report the fraction of injected faults that were detected by Phase 1 rules |

### 1.4 Explicitly Out of Scope

| Excluded Capability | Rationale |
|---|---|
| Hardware fault simulation | Physical layer errors such as CRC failures or signal degradation are outside software analyzer scope |
| Random or probabilistic fault injection | All faults are deterministic and explicitly declared; stochastic injection is not supported |
| Persistent fault state across runs | No injected fault persists beyond a single analysis run; the injection layer is stateless between runs |
| Electrical or PHY-level modeling | Below the scope of a transaction-layer analyzer |

---

## 2. Glossary

| Term | Definition |
|---|---|
| **Fault** | A deliberately introduced abnormality applied to a `RawPacket` or to the packet stream. |
| **Fault Scenario** | A named, reusable collection of one or more faults declared in a JSON configuration file. |
| **Injection Point** | The position in the packet stream — specified by packet index or by a conditional matching rule — at which a fault is applied. |
| **Fault Effect** | The observable change made to the packet or stream by the injection layer as a result of applying the fault. |
| **Expected Detection** | The Phase 1 rule (DEC-xxx or VAL-xxx) or system-level rule (SYS-xxx from Phase 5) that the fault is designed to trigger. |
| **Detection Gap** | A fault that was injected but for which no matching rule was triggered. Indicates a potential blind spot in the detection logic. |
| **Fault Report** | The diagnostic document produced alongside the normal analysis report when fault injection is enabled, listing every injected fault and its detection outcome. |
| **FaultInjectionLayer** | The transparent interceptor component that sits between the packet source and the Phase 1 pipeline and applies fault effects to passing packets. |
| **Static Fault** | A fault whose injection point is a fixed, explicitly declared packet index. |
| **Conditional Fault** | A fault whose injection point is determined at runtime by matching a declared condition — such as TLP type or occurrence count — against passing packets. |
| **FaultLog** | The internal running record of every fault application produced during a single run, used as input to the coverage analysis step. |

---

## 3. Phase Independence Guarantee

**Rule 1 — Disabled by default.** When the `--inject` CLI argument is absent, the `FaultInjectionLayer` is not instantiated and the processing pipeline is indistinguishable from a Phases 1 through 5 execution.

**Rule 2 — RawPacket interception only.** The `FaultInjectionLayer` operates exclusively on `RawPacket` objects. The `PacketDecoder`, `ProtocolValidator`, and `TransactionMapper` are not modified, wrapped, or replaced.

**Rule 3 — No modification of prior phases.** No Phase 1, 2, 3, 4, or 5 source file is modified.

**Rule 4 — Ephemeral fault state.** No fault state persists between analysis runs. The injection layer is constructed fresh from its configuration file at the beginning of each run and is destroyed at the end.

**Rule 5 — Dual report model.** The Phase 1 analysis report records the pipeline's actual behavior — what packets were received and what errors were detected, whether inputs were original or injected. The fault diagnostic report is a separate, additive document. The two never share content.

---

## 4. Fault Modeling Philosophy

Faults in Phase 6 are treated as **deliberate test stimuli**, not as random disturbances. Each fault is designed to be:

**Intentional.** Every fault is explicitly declared in a named scenario configuration. Undeclared faults do not exist.

**Deterministic.** Applying the same scenario configuration to the same trace file always produces the same fault at the same injection point, with the same effect, in every run without exception.

**Observable.** Every injected fault is recorded in the `FaultLog`. Every rule triggered in response is correlated with the injecting fault. Every detection gap — a fault that was not detected — is explicitly flagged.

**Isolated.** A fault affects only the targeted packet or transaction. It must not corrupt analyzer component state, alter the behavior of subsequent non-faulted packets, or leave any residue in the pipeline.

A fault that causes the analyzer to crash, hang, or produce an incoherent report is not a successful negative test. It is a defect in the analyzer's robustness, and it must be fixed.

---

## 5. Functional Requirements

### 5.1 Fault Injection Control

| ID | Requirement |
|---|---|
| FR-INJECT-01 | Fault injection shall be enabled by the presence of the `--inject <scenario_file>` CLI argument. |
| FR-INJECT-02 | When the `--inject` argument is absent, the analysis pipeline shall be byte-for-byte equivalent to a Phases 1 through 5 execution. |
| FR-INJECT-03 | The `FaultInjectionLayer` shall evaluate faults in the order they are defined within the scenario configuration file. |
| FR-INJECT-04 | Multiple faults may target the same packet index. When multiple faults apply to the same packet, all are applied in definition order before the packet is passed downstream. |
| FR-INJECT-05 | Every fault application shall be recorded as a `FaultLogEntry` in the `FaultLog` for use in coverage analysis. |

### 5.2 Packet-Level Faults

| ID | Requirement |
|---|---|
| FR-FAULT-PKT-01 | The system shall support **byte corruption**: overwriting a specified number of bytes at a specified byte offset within `payload_hex`. |
| FR-FAULT-PKT-02 | The system shall support **packet truncation**: shortening `payload_hex` to a specified byte count, discarding all bytes beyond that point. |
| FR-FAULT-PKT-03 | The system shall support **packet duplication**: inserting an identical copy of the targeted packet immediately after the original in the stream. |
| FR-FAULT-PKT-04 | The system shall support **packet suppression**: removing the targeted packet from the stream entirely, so that the Phase 1 pipeline never receives it. |
| FR-FAULT-PKT-05 | The system shall support **field override**: replacing the value of a named TLP header field in the serialized packet payload with a specified replacement value before passing the modified packet downstream. |

### 5.3 Transaction-Level Faults

| ID | Requirement |
|---|---|
| FR-FAULT-TXN-01 | The system shall support **completion suppression**: removing from the stream the CplD or Cpl that corresponds to a specified request transaction, simulating a missing completion (targets VAL-002). |
| FR-FAULT-TXN-02 | The system shall support **completion duplication**: inserting a duplicate of a completion that has already been delivered in the stream, simulating a duplicate completion (targets VAL-003). |
| FR-FAULT-TXN-03 | The system shall support **completion status override**: replacing the `Status` field of a targeted completion with either `UR` or `CA`, simulating an error completion (targets Phase 5 ERROR transaction state). |
| FR-FAULT-TXN-04 | The system shall support **byte count mismatch injection**: replacing the `byte_count` field of a targeted CplD with a value that is inconsistent with the corresponding request's `length_dw` (targets VAL-004). |

### 5.4 Timing and Ordering Faults

| ID | Requirement |
|---|---|
| FR-FAULT-TIME-01 | The system shall support **timestamp override**: replacing the `timestamp_ns` field of a targeted packet with a specified value. |
| FR-FAULT-TIME-02 | The system shall support **packet reordering**: swapping the stream positions of two explicitly specified packets by their indices. |

### 5.5 Fault Coverage Reporting

| ID | Requirement |
|---|---|
| FR-COV-01 | After each run with injection enabled, the system shall produce a `FaultCoverageReport` that maps each injected fault to the Phase 1 rule — or absence of rule — that was triggered in response. |
| FR-COV-02 | Faults for which no matching rule was triggered shall be listed in the `FaultCoverageReport` as `DETECTION_GAP` entries with outcome classification `"UNDETECTED"`. |
| FR-COV-03 | The `FaultCoverageReport` shall include a summary section stating: total faults injected, total faults detected, total detection gaps, and coverage percentage computed as (detected / injected) × 100. |

---

## 6. Non-Functional Requirements

| ID | Category | Requirement |
|---|---|---|
| NFR-INJECT-01 | Determinism | The same fault scenario applied to the same trace must produce identical injection effects, identical detection results, and an identical `FaultCoverageReport` across all runs, environments, and hardware. |
| NFR-INJECT-02 | Isolation | A fault must not alter any analyzer component's state beyond the scope of the faulted packet. After the faulted packet is processed, subsequent non-faulted packets must be processed identically to their unfaulted counterparts. |
| NFR-INJECT-03 | Robustness | No injected fault, regardless of its type or severity, may cause the analyzer to crash, enter an infinite loop, or emit a logically inconsistent report for packets that were not targeted by that fault. |
| NFR-INJECT-04 | Observability | Every injected fault must appear in the `FaultLog` with its injection point, the effect applied, and the detection outcome. No fault may be applied silently. |
| NFR-INJECT-05 | Performance | When injection is enabled, the additional processing overhead introduced by the `FaultInjectionLayer` must not increase total analysis time by more than 10 percent compared to an equivalent run with injection disabled. |

---

## 7. Architectural Design

### 7.1 Injection Layer Position

The `FaultInjectionLayer` is inserted between the packet source and the Phase 1 decoder in both offline and live modes. In offline mode, it sits between the `TraceFileReader` and the `PacketDecoder`. In live mode, it wraps the `QueueManager.push()` call on the producer side.

```
┌────────────────────────────────────────────────────┐
│  Packet Source (TraceFileReader or AsioSession)    │
└─────────────────────┬──────────────────────────────┘
                      │  RawPacket (original)
                      ▼
┌────────────────────────────────────────────────────┐
│  FaultInjectionLayer                               │
│  Evaluates fault conditions.                       │
│  Applies matching fault effects.                   │
│  Records each application in FaultLog.             │
│  Suppresses the packet, or passes it modified.     │
└─────────────────────┬──────────────────────────────┘
                      │  RawPacket (original or modified)
                      │  (or: no packet, if suppressed)
                      ▼
┌────────────────────────────────────────────────────┐
│  Phase 1 Pipeline (PacketDecoder → ProtocolValidator│
│  → ReportBuilder → TransactionMapper)              │
│  Entirely unchanged from Phase 5.                  │
└────────────────────────────────────────────────────┘
                      │
                      ▼
              Phase 1 Analysis Report
                      +
              Fault Coverage Report
```

### 7.2 Coverage Analysis

After the pipeline completes, the `FaultCoverageAnalyzer` component correlates the `FaultLog` against the `DecodeError` and `ValidationError` collections in the Phase 1 report. For each `FaultLogEntry` whose `expected_rule` matches a rule ID present in the error collections for the targeted packet, the outcome is classified as `DETECTED`. For entries where no matching rule ID is found, the outcome is `UNDETECTED` (a detection gap). The `FaultCoverageReport` is then serialized to the path specified by `--fault-report`.

---

## 8. Fault Taxonomy

### 8.1 Structural Faults (STRUCT)

Structural faults target the binary layout of the packet, producing decode errors when processed by the `PacketDecoder`.

| Fault ID | Type | Target Field | Expected Rule |
|---|---|---|---|
| STRUCT-001 | FIELD_OVERRIDE | `Fmt [31:29]` | DEC-001 |
| STRUCT-002 | FIELD_OVERRIDE | `Type [28:24]` | DEC-002 |
| STRUCT-003 | FIELD_OVERRIDE | `TC [22:20]` | DEC-003 |
| STRUCT-004 | FIELD_OVERRIDE | `Length [9:0]` (set to 0) | DEC-004 |
| STRUCT-005 | PACKET_TRUNCATE | Payload shorter than minimum header | DEC-005 |
| STRUCT-006 | FIELD_OVERRIDE | `Address [1:0]` (3DW, set non-zero) | DEC-006 |
| STRUCT-007 | FIELD_OVERRIDE | `Address [1:0]` (4DW, set non-zero) | DEC-007 |
| STRUCT-008 | FIELD_OVERRIDE | `Status [15:13]` (invalid code) | DEC-008 |
| STRUCT-009 | BYTE_CORRUPT | `payload_hex` (inject non-hex character) | DEC-009 |

### 8.2 Protocol Faults (PROTO)

Protocol faults produce structurally valid packets that violate transaction-level rules, triggering validation errors in the `ProtocolValidator`.

| Fault ID | Type | Effect | Expected Rule |
|---|---|---|---|
| PROTO-001 | COMPLETION_INJECT | Insert a CplD with no prior matching MRd | VAL-001 |
| PROTO-002 | COMPLETION_SUPPRESS | Remove the CplD for a targeted MRd | VAL-002 |
| PROTO-003 | COMPLETION_DUPLICATE | Insert a second CplD for an already-completed transaction | VAL-003 |
| PROTO-004 | BYTE_COUNT_OVERRIDE | Set CplD byte_count to an inconsistent value | VAL-004 |
| PROTO-005 | TAG_REUSE | Issue a second MRd with a tag already in use | VAL-005 |

### 8.3 Timing and Ordering Faults (TIME)

| Fault ID | Type | Effect | Expected Detection |
|---|---|---|---|
| TIME-001 | TIMESTAMP_OVERRIDE | Replace timestamp_ns on a targeted packet | Phase 5 latency anomaly |
| TIME-002 | PACKET_REORDER | Swap stream positions of two packets | VAL-001 or VAL-002 depending on swap pair |

---

## 9. Component Specifications

### 9.1 FaultInjectionLayer

**Single Responsibility:** Intercept each `RawPacket` as it transitions from the packet source to the analysis pipeline. Check whether any declared fault targets the current packet. Apply matching fault effects. Record each application in the `FaultLog`. Pass the modified packet, or suppress it entirely.

**Behavioral Contract:**
- Constructed from a parsed `FaultScenario` and an empty `FaultLog`.
- Provides a `process(RawPacket) → optional<RawPacket>` operation. Returns a modified `RawPacket` if the packet is to be passed forward; returns empty if the packet is to be suppressed; may return multiple packets if duplication is applied.
- Is stateless between packets beyond the fault configuration and the accumulating `FaultLog`.
- Never throws an exception for any input. Any fault application error is logged and the original packet is passed through unmodified.

### 9.2 FaultScenarioLoader

**Single Responsibility:** Parse a fault scenario JSON file and produce a validated `FaultScenario` object.

**Behavioral Contract:**
- Validates the `schema_version` field of the scenario file.
- Validates the syntax of all injection point specifications.
- Raises `FaultScenarioParseError` for any malformed scenario file, providing a descriptive message identifying the specific parsing failure.

### 9.3 FaultCoverageAnalyzer

**Single Responsibility:** Correlate the `FaultLog` against the Phase 1 report's error collections and produce the `FaultCoverageReport`.

**Behavioral Contract:**
- Accepts the completed `FaultLog` and the final Phase 1 report as inputs.
- For each `FaultLogEntry`, searches the Phase 1 report's decode errors and validation errors for a rule matching `expected_rule` on the targeted packet index.
- Classifies the outcome as `DETECTED` or `UNDETECTED`.
- Computes coverage statistics and serializes the report.

---

## 10. Data Structures

### 10.1 FaultLogEntry

| Field | Type | Description |
|---|---|---|
| `fault_id` | `string` | The identifier from the scenario configuration (e.g., `"STRUCT-001"`) |
| `target_packet_index` | `uint64_t` | The original (pre-injection) packet index of the targeted packet |
| `effect` | `string` | Human-readable description of the effect applied |
| `expected_rule` | `string` | The rule ID expected to be triggered |
| `outcome` | `string` | `"DETECTED"`, `"UNDETECTED"`, or `"NOT_APPLIED"` (if the injection point was not reached) |
| `triggered_rule` | `string` | The rule ID that was actually triggered; empty string if none |

### 10.2 FaultCoverageReport Fields

| Field | Type | Description |
|---|---|---|
| `scenario_id` | `string` | The `scenario_id` from the configuration file |
| `trace_file` | `string` | Path of the analyzed trace file |
| `total_faults_injected` | `uint32_t` | Total faults that were applied |
| `detected` | `uint32_t` | Faults for which the expected rule was triggered |
| `detection_gaps` | `uint32_t` | Faults for which no matching rule was triggered |
| `coverage_percent` | `double` | (detected / total_faults_injected) × 100 |
| `entries` | `FaultLogEntry[]` | Complete list of per-fault outcomes |

---

## 11. Fault Configuration Specification

### 11.1 Scenario File Structure

A fault scenario file is a JSON document with the following top-level fields:

**`schema_version`** — The scenario file schema version, currently `"1.0"`. The `FaultScenarioLoader` validates this field before processing any other content.

**`scenario_id`** — A unique, human-readable identifier for the scenario. Used in the fault report output.

**`description`** — A free-text description of the scenario's purpose.

**`faults`** — An ordered array of fault definition objects.

### 11.2 Fault Definition Object

Each fault definition contains:

**`fault_id`** — A unique identifier string within the scenario, used in the `FaultLog` and `FaultCoverageReport`.

**`type`** — The fault type from the taxonomy in Section 8: `FIELD_OVERRIDE`, `BYTE_CORRUPT`, `PACKET_TRUNCATE`, `PACKET_DUPLICATE`, `PACKET_SUPPRESS`, `COMPLETION_SUPPRESS`, `COMPLETION_DUPLICATE`, `BYTE_COUNT_OVERRIDE`, `TIMESTAMP_OVERRIDE`, or `PACKET_REORDER`.

**`injection_point`** — Specifies when in the stream this fault activates. Two sub-types are supported:
- `BY_INDEX`: activates when the stream packet count equals the specified `packet_index`.
- `BY_TYPE`: activates on the Nth occurrence (specified by `occurrence`) of a packet whose TLP type matches the specified `tlp_type`.

**`expected_rule`** — The Phase 1 rule ID (DEC-xxx, VAL-xxx, or SYS-xxx) expected to be triggered by this fault. Used by the coverage analyzer.

**Type-specific parameters** — Additional fields as required by the fault type: `field_name` and `field_value` for FIELD_OVERRIDE; `byte_offset` and `replacement_bytes` for BYTE_CORRUPT; `target_length` for PACKET_TRUNCATE; and so on.

### 11.3 Scenario Index References

All `packet_index` values in a fault definition refer to the **original pre-injection** packet indices as they appear in the source trace. They do not refer to the post-injection stream position, which may differ when packets have been suppressed, duplicated, or reordered by prior faults. The `FaultInjectionLayer` maintains an original-index counter that is incremented for each packet it receives from the source, regardless of what it does with that packet.

---

## 12. Fault Lifecycle

The execution lifecycle for a fault-injection-enabled analysis run proceeds as follows:

1. At startup, the `FaultScenarioLoader` reads and validates the scenario file. If the file is malformed, a `FaultScenarioParseError` is raised and analysis is aborted with exit code 3.
2. The `FaultInjectionLayer` is constructed from the validated `FaultScenario` and inserted into the processing pipeline.
3. For each packet emitted by the source, the `FaultInjectionLayer` evaluates all fault definitions in order. For each matching fault, the fault effect is applied to the packet and a `FaultLogEntry` is recorded.
4. The modified packet (or no packet, if suppressed) is passed to the Phase 1 pipeline. Duplicated packets produce multiple downstream deliveries.
5. After all source packets are processed and the Phase 1 pipeline finalizes, the `FaultCoverageAnalyzer` receives the `FaultLog` and the Phase 1 report.
6. The `FaultCoverageAnalyzer` performs outcome classification for each fault and computes coverage statistics.
7. The `FaultCoverageReport` is serialized to the path specified by `--fault-report`.
8. The Phase 1 analysis report is serialized to the path specified by `--output`.

Both reports are always written if the analysis completes without an exception. Neither report's write failure prevents the other from being attempted.

---

## 13. Diagnostic Reporting

### 13.1 FaultCoverageReport Structure

The fault coverage report is a standalone JSON document produced in addition to the Phase 1 analysis report. It is written to the path specified by the `--fault-report` CLI argument.

The report contains a summary section with the scenario ID, trace file path, total faults injected, detected count, detection gap count, and coverage percentage. It then contains a complete `entries` array with one `FaultLogEntry` per injected fault, providing full traceability from scenario definition to detection outcome.

### 13.2 CLI Integration

| New Argument | Required When | Description |
|---|---|---|
| `--inject <path>` | Never (optional) | Path to the fault scenario JSON file. Enables fault injection mode for the run. |
| `--fault-report <path>` | When `--inject` is present | Path for the fault diagnostic report output. |

---

## 14. GUI Integration

When Phase 2 is present and fault injection is enabled, the following display enhancements are applied. All enhancements are read-only; the GUI reads fault data exclusively from the fault report JSON file.

**Injected packets** are marked in the packet table with a dedicated visual indicator — distinct from the malformed-packet and validation-error indicators — identifying them as fault-injected.

**Suppressed packets** appear as placeholder rows in the table with a suppression indicator, preserving the visual continuity of the index sequence.

**Fault log panel** — A new panel listing every fault applied during the run, including its fault ID, target packet index, effect description, and detection outcome. Detection gaps are highlighted with a prominent warning indicator.

---

## 15. Testing Strategy

### 15.1 Fault Coverage Tests

One test per fault in the taxonomy (Section 8), comprising sixteen tests in total. Each test:
1. Constructs a scenario containing exactly one fault definition targeting a known-good canonical trace.
2. Runs analysis with injection enabled.
3. Verifies that the expected rule ID appears in the Phase 1 report for the targeted packet.
4. Verifies that the `FaultCoverageReport` records the outcome as `"DETECTED"` for that fault.

### 15.2 Regression Safety Tests

For every canonical Phase 1 trace, a regression safety test executes the analyzer without the `--inject` flag and confirms that the output is byte-identical to the Phase 1 baseline. This verifies that the injection layer has zero side effects when not active.

### 15.3 Negative Tests

| Test | Expected Outcome |
|---|---|
| Scenario file is malformed JSON | `FaultScenarioParseError` raised; analysis aborted with exit code 3 |
| Fault targeting a packet index beyond the end of the trace | Fault recorded as `NOT_APPLIED` in the fault report; no error produced |
| Any injected fault that would cause the analyzer to crash | Must not occur; NFR-INJECT-03 requirement; treat as a blocking defect |
| Detection gap scenario | A fault is injected that deliberately has no corresponding rule; the fault report records outcome as `"UNDETECTED"` and the gap count is 1 |

---

## 16. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation Strategy |
|---|---|---|---|
| An injected fault corrupts the decoder's state for packets that follow | Low | High | The `PacketDecoder` is stateless across packets by design. Per-packet state corruption is structurally impossible. Verify with unit tests that show correct decoding of packets after a faulted packet. |
| A detection gap goes unnoticed during routine CI | Medium | Medium | The `FaultCoverageReport` is parsed as part of the CI pipeline. Any entry with `outcome == "UNDETECTED"` triggers a CI failure unless the test is explicitly marked as a gap-verification test. |
| Scenario file format changes break existing scenario files | Low | Medium | Scenario files include a `schema_version` field. The `FaultScenarioLoader` validates the version and rejects files with unsupported versions. When the schema changes, the version is incremented. |
| Packet suppression shifts original indices for all subsequent packets | Medium | Medium | Injection point indices always refer to original source indices, tracked by the `FaultInjectionLayer` independently of the modified stream. This is documented explicitly in Section 11.3. |

---

## 17. Exit Criteria

Phase 6 is considered complete and accepted when all of the following conditions are simultaneously satisfied:

- [ ] All sixteen faults in the taxonomy (STRUCT-001 through TIME-002) trigger their expected Phase 1 rules, verified by individual fault coverage tests.
- [ ] The `FaultCoverageReport` correctly reports 100 percent coverage for the full-coverage scenario.
- [ ] At least one detection gap test case is correctly identified and reported as `"UNDETECTED"` in the fault report.
- [ ] Fault injection disabled mode produces byte-identical output to Phase 1 through 5 for all canonical trace files.
- [ ] No injected fault from any test scenario causes the analyzer to crash, hang, or produce a logically inconsistent report for non-targeted packets.
- [ ] The fault scenario file format is documented with at minimum three example scenarios covering structural faults, protocol faults, and a mixed scenario.
- [ ] All unit and system tests pass with zero failures.
- [ ] Phase 1 through 5 components remain entirely unmodified and fully operational.

