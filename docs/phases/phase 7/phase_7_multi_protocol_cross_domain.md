# Phase 7 — Multi-Protocol and Cross-Domain Modeling

**Document ID:** PCIE-SPEC-PHASE-07  
**Project:** Traceon  
**Version:** 2.0.0  
**Status:** Approved  
**Phase Dependencies:** Phase 1 (PCIE-SPEC-PHASE-01) — the PCIe domain is the reference implementation of the protocol abstraction interface. Phase 5 (PCIE-SPEC-PHASE-05) — the `Transaction` model and `VirtualComponentRegistry` are extended as the foundation for cross-domain correlation. Phases 2, 3, and 4 are optional; their integrations are described in their respective sections.  
**Consumed By:** No subsequent phase is defined. Phase 7 is the terminal extension phase of the Traceon platform.

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Glossary](#2-glossary)
3. [Phase Independence Guarantee](#3-phase-independence-guarantee)
4. [Design Philosophy](#4-design-philosophy)
5. [Functional Requirements](#5-functional-requirements)
6. [Non-Functional Requirements](#6-non-functional-requirements)
7. [Architectural Design](#7-architectural-design)
8. [Protocol Abstraction Layer](#8-protocol-abstraction-layer)
9. [Domain Registry](#9-domain-registry)
10. [Cross-Domain Modeling Layer](#10-cross-domain-modeling-layer)
11. [Unified Address Space Model](#11-unified-address-space-model)
12. [Global Logical Timeline](#12-global-logical-timeline)
13. [Cross-Domain Validation Rules](#13-cross-domain-validation-rules)
14. [Error Propagation Modeling](#14-error-propagation-modeling)
15. [Component Specifications](#15-component-specifications)
16. [Data Structures](#16-data-structures)
17. [Reporting and Visualization](#17-reporting-and-visualization)
18. [Protocol Registration Guide](#18-protocol-registration-guide)
19. [Testing Strategy](#19-testing-strategy)
20. [Risks and Mitigations](#20-risks-and-mitigations)
21. [Exit Criteria](#21-exit-criteria)

---

## 1. Purpose and Scope

### 1.1 Purpose

Phase 7 transforms the Traceon platform from a single-protocol PCIe analysis engine into a **multi-protocol, cross-domain analysis platform**. It introduces a unified protocol abstraction layer through which multiple interconnect and storage protocols — PCIe, CXL, NVMe, and future additions — can coexist within a single analysis session. A new cross-domain correlation engine detects system-level violations that are only observable when traffic from multiple protocol domains is considered simultaneously. Full backward compatibility is preserved: PCIe-only analysis remains the default and is completely unaffected by the introduction of multi-domain infrastructure.

### 1.2 Objectives

- Define a `ProtocolDomain` abstract interface that allows PCIe, CXL, NVMe, and custom protocols to be registered as equal participants in the platform without modifying any existing component.
- Enable simultaneous ingestion, parsing, and analysis of trace data from multiple protocol domains within a single run.
- Provide a cross-domain correlation engine that identifies relationships between transactions in different domains and reports violations that span protocol boundaries.
- Maintain a unified address space model covering all active domains and detecting unauthorized cross-domain memory accesses.
- Construct a global logical timeline that merges and orders events from all active domains for system-level temporal analysis.
- Extend the Traceon report schema to version `3.0` with per-domain sections, cross-domain correlation results, and error propagation chains.

### 1.3 In-Scope Capabilities

| Capability | Description |
|---|---|
| Protocol abstraction layer | A common abstract interface implemented independently by each protocol domain |
| Multi-protocol trace ingestion | Simultaneous ingestion and per-domain analysis from separate trace sources |
| Cross-domain transaction correlation | Matching of transactions in one domain to causally related transactions in another |
| Unified address space model | A logical address map covering all active domains with ownership and permission annotations |
| Global logical timeline | A merged, globally ordered sequence of events from all domains |
| Cross-domain validation | Detection of violations observable only by comparing events across domain boundaries |
| Error propagation modeling | Causal chain analysis linking an error in one domain to dependent failures in other domains |

### 1.4 Explicitly Out of Scope

| Excluded Capability | Rationale |
|---|---|
| PCIe Physical or Data Link layer modeling | Below the transaction layer; outside analyzer scope |
| OS or driver-level modeling | Software stack above the protocol boundary |
| Full system emulation | Simulation of CPU cores, memory controllers, or DMA engines |
| Hardware resource arbitration | Requires platform-specific topology knowledge |
| Electrical or PHY-level interaction modeling | Below the scope of a transaction-layer platform |

---

## 2. Glossary

| Term | Definition |
|---|---|
| **Protocol Domain** | An isolated, self-contained namespace encapsulating all packet parsing, transaction modeling, and validation logic for one protocol. |
| **Domain ID** | A stable, lowercase string identifier uniquely naming a registered protocol domain. Examples: `"pcie"`, `"cxl"`, `"nvme"`. |
| **ProtocolDomain Interface** | The abstract C++ base class that every domain must implement. Defines the contract through which the platform orchestrates domain-specific operations. |
| **DomainRegistry** | The platform component that holds references to all registered domains and provides discovery services. |
| **Cross-Domain Layer** | The component responsible for evaluating correlation rules, maintaining the unified address map, constructing the global timeline, and detecting cross-domain violations. |
| **Correlation Rule** | A declarative, named rule that defines how a transaction in one domain may be matched to a related transaction in another domain, based on address, timing, or identifier relationships. |
| **CrossDomainMatch** | A record produced by a correlation rule when it successfully links a transaction in a source domain to a corresponding transaction in a target domain. |
| **Unified Address Map** | A logical model of address ranges across all active domains, annotated with domain ownership and access permissions. |
| **Global Timeline** | A merged, timestamp-ordered sequence of events from all active domains, enabling system-level temporal analysis. |
| **Cross-Domain Violation** | A protocol violation detectable only by observing events from two or more domains simultaneously. |
| **Error Propagation Chain** | A directed causal chain linking an error event in one domain (root cause) to dependent failures in other domains (consequences). |
| **Protocol Neutrality** | The architectural principle that no domain is privileged over any other. PCIe is the reference implementation, not the default or the master. |

---

## 3. Phase Independence Guarantee

**Rule 1 — Existing protocol implementations are not modified.** The PCIe domain implementation (Phase 1) is not changed. CXL and NVMe domains are entirely additive new components.

**Rule 2 — No domain depends on any other domain.** PCIe-only operation remains complete and correct when no other domain is registered. The platform infrastructure does not require a minimum number of domains.

**Rule 3 — Cross-domain logic is inactive with one domain.** When only one domain is active, the `CrossDomainLayer` is instantiated but produces no output. It does not affect the per-domain analysis quality in any way.

**Rule 4 — Phase 7 is fully disablable.** Removing or disabling all Phase 7 components returns the system to Phase 1 through 6 behavior exactly. Single-protocol PCIe analysis is the default.

**Rule 5 — No Phase 1 through 6 source file is modified.**

---

## 4. Design Philosophy

Phase 7 is governed by four architectural principles that must be honored throughout all implementation work.

### 4.1 Protocol Neutrality

No domain is privileged or treated as a special case. PCIe is the reference implementation of the `ProtocolDomain` interface — the first implementation and the model against which the interface was designed — but it is not architecturally superior to CXL, NVMe, or any future domain. All domains are registered, discovered, and orchestrated identically.

### 4.2 Domain Isolation

A domain's internal parser, validator, and transaction mapper are invisible to all other domains. Domains communicate only through the cross-domain layer's well-defined correlation contracts. No domain may call another domain's methods directly, read another domain's internal state, or depend on another domain's presence. Domain isolation must be enforceable through normal C++ access control: no domain exposes internals through public interfaces.

### 4.3 Explicit Interaction Contracts

Every relationship between domains must be expressed as a named, versioned correlation rule in the correlation rule configuration. Implicit or emergent relationships — behaviors that arise from implementation coincidences rather than declared intent — are not permitted. If two domains interact, the interaction must be codified as a rule. If it is not in the configuration, it is not a valid interaction.

### 4.4 Incremental Extensibility

Adding a new protocol domain must require only: implementing the `ProtocolDomain` interface, registering the domain with the `DomainRegistry`, and optionally defining correlation rules and address regions involving the new domain. It must not require modifying any existing domain, the cross-domain engine, the platform orchestrator, or any prior phase component.

---

## 5. Functional Requirements

### 5.1 Protocol Abstraction

| ID | Requirement |
|---|---|
| FR-MULTI-01 | The system shall define a `ProtocolDomain` abstract interface as specified in Section 8. All operations that the platform performs on a domain shall be expressed through this interface. |
| FR-MULTI-02 | The PCIe domain shall implement the `ProtocolDomain` interface without any modification to its existing Phase 1 analysis logic. The interface is an additive wrapper, not a replacement. |
| FR-MULTI-03 | The system shall provide a `DomainRegistry` through which domains are registered at startup and discovered by the platform orchestrator during execution. |
| FR-MULTI-04 | Each registered domain shall declare a unique, stable `domain_id` string. Duplicate `domain_id` registration shall be rejected at startup with a descriptive error. |

### 5.2 Multi-Protocol Ingestion

| ID | Requirement |
|---|---|
| FR-MULTI-05 | The system shall support loading separate trace files for each active domain, specified through domain-specific CLI arguments or a platform configuration file. |
| FR-MULTI-06 | Each domain shall process its own trace file entirely through its own parsing and validation components. No domain's trace data is shared with or processed by another domain's components. |
| FR-MULTI-07 | Per-domain analysis for all active domains shall complete before cross-domain correlation begins. Cross-domain correlation is a post-processing step, not an in-stream operation. |
| FR-MULTI-08 | In live mode (Phase 4), each domain may receive packets on a separate TCP port with its own `AsioSession` and queue. |

### 5.3 Cross-Domain Correlation

| ID | Requirement |
|---|---|
| FR-CROSS-01 | After per-domain analysis is complete for all domains, the system shall evaluate all registered correlation rules against the resulting transaction sets. |
| FR-CROSS-02 | Each correlation rule evaluation shall produce zero or more `CrossDomainMatch` records, each linking a specific transaction in a source domain to a specific transaction in a target domain. |
| FR-CROSS-03 | Correlation rules shall be declarative — defined in a configuration file, not hardcoded in the cross-domain engine. The engine evaluates rules; it does not define them. |
| FR-CROSS-04 | Transactions for which no correlation rule finds a counterpart shall be listed in the cross-domain section of the report as `UNMATCHED`. An unmatched transaction is not inherently an error; it becomes one only if a rule declares that a match is required. |

### 5.4 Unified Address Space

| ID | Requirement |
|---|---|
| FR-ADDR-01 | The system shall maintain a unified address map in which address ranges are registered with an owning domain and optional access permission annotations. |
| FR-ADDR-02 | The system shall detect when a transaction in one domain accesses an address range owned by a different domain without a declared cross-domain access permission. This constitutes a cross-domain access violation. |
| FR-ADDR-03 | Address range ownership and permissions shall be configurable through a platform address map configuration file, independent of the trace inputs. |

### 5.5 Global Logical Timeline

| ID | Requirement |
|---|---|
| FR-TIME-01 | The system shall construct a global timeline by merging the timestamped events from all active domains into a single ordered sequence. |
| FR-TIME-02 | Events from all domains shall be ordered by `timestamp_ns`. For events with identical timestamps, the ordering within that timestamp shall be by domain ID in lexicographic order. |
| FR-TIME-03 | When timestamps are unavailable (all zero) for any domain, the global timeline shall be constructed from intra-domain ordering only, and cross-domain temporal comparisons shall be marked as unavailable. |

### 5.6 Cross-Domain Validation

| ID | Requirement |
|---|---|
| FR-XVAL-01 | The system shall evaluate all cross-domain validation rules defined in Section 13 after correlation is complete. |
| FR-XVAL-02 | Cross-domain violations shall be emitted as `CrossDomainViolation` records and included in the `cross_domain.violations` section of the report. They are distinct from per-domain `ValidationError` and `SystemViolation` records. |
| FR-XVAL-03 | Cross-domain violations shall not modify, replace, or suppress any per-domain violations. All populations coexist independently in the report. |

---

## 6. Non-Functional Requirements

| ID | Category | Requirement |
|---|---|---|
| NFR-MULTI-01 | Backward Compatibility | Running in single-domain PCIe mode must produce reports that are byte-identical to Phase 1 through 6 output. The presence of Phase 7 infrastructure must have zero effect on single-protocol correctness. |
| NFR-MULTI-02 | Domain Isolation | A failure in any one domain's analysis must not propagate to any other domain or to the cross-domain layer. Per-domain failures are captured, reported, and isolated. |
| NFR-MULTI-03 | Extensibility | Adding a new protocol domain, per the procedure in Section 18, must require zero changes to any existing domain or to the cross-domain engine source. |
| NFR-MULTI-04 | Performance — Correlation | Cross-domain correlation must complete in O(N log N) time or better with respect to the total transaction count across all domains. For a combined workload of 30,000 transactions across three domains, correlation must complete within five seconds on the reference system. |
| NFR-MULTI-05 | Correctness | Single-protocol PCIe regression tests must pass with zero output differences before any multi-domain feature is considered complete. |

---

## 7. Architectural Design

### 7.1 System-Level Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      Platform Orchestrator                      │
│  Reads configuration. Instantiates domains. Coordinates phases. │
├──────────────────┬──────────────────┬───────────────────────────┤
│   PCIe Domain    │    CXL Domain    │     NVMe Domain (+ more)  │
│  (Phase 1 core)  │  (new in Ph. 7)  │     (new in Ph. 7)        │
│  PacketDecoder   │  CxlDecoder      │     NvmeDecoder           │
│  ProtocolVal.    │  CxlValidator    │     NvmeValidator         │
│  TransactionMap  │  CxlTxnMapper    │     NvmeTxnMapper         │
├──────────────────┴──────────────────┴───────────────────────────┤
│                     Cross-Domain Layer                          │
│  CorrelationEngine   UnifiedAddressMap   GlobalTimeline         │
│  CrossDomainValidator   ErrorPropagationAnalyzer                │
├─────────────────────────────────────────────────────────────────┤
│                     Reporting Layer                             │
│  Per-domain reports (v1.0/v2.0) + Cross-domain section (v3.0)  │
└─────────────────────────────────────────────────────────────────┘
```

### 7.2 Analysis Sequencing

Phase 7 analysis proceeds in two distinct sequential phases:

**Phase A — Per-Domain Analysis:** Each registered domain independently completes its full packet decoding, protocol validation, and transaction mapping on its own trace data. The domains operate concurrently (if Phase 4 infrastructure is available) or sequentially. No cross-domain communication occurs during this phase.

**Phase B — Cross-Domain Analysis:** After all domains complete Phase A, the `CrossDomainLayer` receives the complete transaction sets from all domains. It evaluates correlation rules, constructs the unified address map and global timeline, evaluates cross-domain validation rules, and performs error propagation analysis. This phase is strictly sequential and operates on finalized, immutable per-domain data.

---

## 8. Protocol Abstraction Layer

### 8.1 ProtocolDomain Interface

The `ProtocolDomain` abstract interface defines the complete contract that the platform requires from any domain. Every domain must implement all pure virtual methods.

**Identification:**
- `domain_id() → string` — Returns the stable domain identifier string.
- `display_name() → string` — Returns a human-readable name for display purposes.

**Ingestion:**
- `create_trace_source(path) → TraceSourceHandle` — Creates a domain-specific trace reader for the given file path.
- `create_live_session(port) → LiveSessionHandle` — Creates a domain-specific live ingestion session on the given TCP port. Optional; may return null for domains that do not support live ingestion.

**Analysis:**
- `create_decoder() → PacketDecoderHandle` — Creates an instance of the domain's packet decoder.
- `create_validator() → ValidatorHandle` — Creates an instance of the domain's protocol validator.
- `create_transaction_mapper() → TransactionMapperHandle` — Creates an instance of the domain's transaction mapper.

**Results:**
- `get_transactions() → AbstractTransaction[]` — Returns the complete set of transactions reconstructed during analysis.
- `get_validation_errors() → ValidationError[]` — Returns all per-domain protocol violations.
- `get_report_section() → ReportSection` — Returns the serialized per-domain report section for inclusion in the multi-protocol report.

### 8.2 AbstractTransaction

Domains expose their transaction data through an `AbstractTransaction` interface that is common across all protocols. This allows the cross-domain layer to reason about transactions from any domain without knowing their internal structure.

| Field | Type | Description |
|---|---|---|
| `domain_id` | `string` | The domain that owns this transaction |
| `local_txn_id` | `uint64_t` | Transaction identifier local to the owning domain |
| `type_name` | `string` | Protocol-specific transaction type name |
| `address` | `string` | Primary address associated with the transaction |
| `start_time_ns` | `uint64_t` | Timestamp of the transaction's first packet |
| `end_time_ns` | `uint64_t` or null | Timestamp of the transaction's completion event |
| `is_complete` | `bool` | True if the transaction reached a terminal completed state |
| `is_error` | `bool` | True if the transaction terminated with an error |

---

## 9. Domain Registry

### 9.1 Registration Mechanics

The `DomainRegistry` is a singleton component instantiated by the platform orchestrator at startup. Domains are registered by calling `DomainRegistry.register_domain(shared_ptr<ProtocolDomain>)`. Registration order does not affect behavior. The registry validates that each domain's `domain_id()` is unique and rejects duplicate registrations with a startup error.

### 9.2 Discovery

The platform orchestrator uses `DomainRegistry.get_all_domains()` to obtain references to all registered domains. It then drives the Phase A analysis loop by calling each domain's ingestion and analysis methods in sequence. The cross-domain layer receives the complete domain set from the registry at the start of Phase B.

### 9.3 Configuration-Driven Domain Activation

Not all registered domains need to be active in every run. The platform configuration file specifies which domains are active for a given analysis session. Inactive registered domains are not instantiated, do not process any trace data, and do not appear in the report.

---

## 10. Cross-Domain Modeling Layer

### 10.1 Correlation Engine

The `CorrelationEngine` reads all registered correlation rules from the configuration file and evaluates each rule against the transaction sets of the domains it references.

A correlation rule specifies:
- A source domain and a condition on source transactions (such as "any NVMe WRITE command targeting an address in range R").
- A target domain and a condition on target transactions (such as "a PCIe MWr to the NVMe doorbell register address derived from R").
- A matching criterion linking the two conditions (such as address derivation, timing proximity, or identifier correspondence).
- A confidence level that the rule assigns to produced matches.

The engine produces `CrossDomainMatch` records for all satisfied rule conditions and `UNMATCHED` records for transactions that no rule was able to pair.

### 10.2 Correlation Rule Evaluation Strategy

The engine must evaluate correlation rules efficiently to meet NFR-MULTI-04. Each rule is evaluated using indexed lookups over the transaction sets rather than exhaustive pairwise comparison. Transactions are indexed by address range and time window at the start of Phase B, enabling O(log N) lookup per candidate pair. The total correlation complexity is O(N log N) where N is the combined transaction count across all active domains.

---

## 11. Unified Address Space Model

### 11.1 Address Map Structure

The unified address map is constructed from a platform configuration file (typically `address_map.json`) that is loaded at startup. It contains a list of address range registrations, each specifying:

- `domain_id` — The domain that owns this address range.
- `base_address` — The lowest address in the range.
- `size` — The byte length of the range.
- `permissions` — A set of domain IDs that are permitted to access this range from cross-domain transactions. An empty set means no cross-domain access is permitted.
- `description` — A human-readable label for the range.

### 11.2 Cross-Domain Access Detection

During cross-domain analysis, every transaction from every domain is checked against the unified address map. If a transaction accesses an address owned by a different domain and that domain is not listed in the transaction's origin domain's permissions for that range, an `XDOM-002` cross-domain access violation is emitted.

---

## 12. Global Logical Timeline

### 12.1 Construction

The global timeline is a merged, timestamp-ordered sequence constructed from all per-domain event sequences. Each event in the timeline carries: the originating domain ID, the local transaction ID within that domain, the event type (request, completion, error, timeout), and the `timestamp_ns`.

### 12.2 Use in Correlation

The global timeline serves the correlation engine by providing a time-ordered view of all system activity. Correlation rules that match events based on temporal proximity use the timeline to find candidate pairs within a configurable time window, avoiding full pairwise search.

### 12.3 Temporal Gap Detection

The global timeline enables detection of situations where events in one domain that should precede events in another domain arrive out of expected order. This enables correlation rules to detect timing violations such as a PCIe transaction completing after the NVMe command that depended on it was already reported complete.

---

## 13. Cross-Domain Validation Rules

| Rule ID | Category | Trigger Condition |
|---|---|---|
| XDOM-001 | `MISSING_CORRELATION` | A transaction in a domain is expected to have a corresponding counterpart in another domain (as declared by a required correlation rule), but no matching transaction was found. |
| XDOM-002 | `ILLEGAL_CROSS_DOMAIN_ACCESS` | A transaction accesses an address range owned by a domain other than the originating domain, and no cross-domain access permission exists for this pair. |
| XDOM-003 | `UNMATCHED_DEPENDENT_TRANSACTION` | A transaction is present in a domain that depends on a transaction in another domain (per a declared dependency rule), but the dependency transaction is absent, failed, or incomplete. |
| XDOM-004 | `TEMPORAL_ORDERING_VIOLATION` | A transaction in domain A that should logically precede a transaction in domain B (per a declared ordering rule) has a later `end_time_ns` than the domain B transaction's `start_time_ns`. |
| XDOM-005 | `ERROR_PROPAGATION_DETECTED` | An error-terminated transaction in one domain is correlated to one or more transactions in other domains that subsequently terminated with errors, forming a causal error propagation chain. |

---

## 14. Error Propagation Modeling

### 14.1 Propagation Chain Analysis

The `ErrorPropagationAnalyzer` operates on the correlated transaction set after all cross-domain matches are produced. For each error transaction, it traces outward through the correlation graph to find all other transactions — in any domain — whose completion or error status is plausibly caused by the original error.

A transaction B is considered a dependent of transaction A if:
- A `CrossDomainMatch` links A and B under a dependency-type correlation rule.
- Transaction B entered an error or incomplete state after transaction A entered an error state.
- The temporal ordering is consistent with a causal relationship (B's failure timestamp follows A's error timestamp on the global timeline).

### 14.2 Propagation Chain Structure

Each propagation chain has one root cause — the earliest error in the chain — and a directed list of dependents. Chains are included in the report's `cross_domain.propagation_chains` section. A transaction may appear in multiple chains if multiple error sources contribute to its failure.

---

## 15. Component Specifications

### 15.1 PlatformOrchestrator

**Single Responsibility:** Own the complete multi-domain analysis lifecycle. Instantiate the `DomainRegistry` and all registered domains. Drive Phase A per-domain analysis. Invoke the `CrossDomainLayer` for Phase B. Assemble and write the multi-protocol report.

### 15.2 CorrelationEngine

**Single Responsibility:** Load and evaluate correlation rules from configuration. Index transaction sets for efficient matching. Produce `CrossDomainMatch` and `UNMATCHED` records.

### 15.3 UnifiedAddressMap

**Single Responsibility:** Load and maintain the platform address range registry. Evaluate cross-domain access permission for any given (transaction, address) pair. Emit XDOM-002 violations.

### 15.4 GlobalTimeline

**Single Responsibility:** Merge per-domain event sequences into a globally ordered timeline. Support time-window queries for correlation rule evaluation.

### 15.5 ErrorPropagationAnalyzer

**Single Responsibility:** Traverse the correlated transaction graph to identify causal error propagation chains. Produce propagation chain records for the report.

---

## 16. Data Structures

### 16.1 CrossDomainMatch

| Field | Type | Description |
|---|---|---|
| `rule_id` | `string` | The correlation rule that produced this match |
| `source_domain` | `string` | Domain ID of the source transaction |
| `source_txn_id` | `uint64_t` | Local transaction ID in the source domain |
| `target_domain` | `string` | Domain ID of the target transaction |
| `target_txn_id` | `uint64_t` | Local transaction ID in the target domain |
| `confidence` | `double` | Rule-assigned match confidence, range 0.0 to 1.0 |

### 16.2 CrossDomainViolation

| Field | Type | Description |
|---|---|---|
| `rule_id` | `string` | The cross-domain rule ID (XDOM-001 through XDOM-005) |
| `category` | `string` | Symbolic violation category |
| `primary_domain` | `string` | Domain ID of the primary transaction involved |
| `primary_txn_id` | `uint64_t` | Local transaction ID in the primary domain |
| `dependent_domain` | `string` or null | Domain ID of the dependent transaction, if applicable |
| `dependent_txn_id` | `uint64_t` or null | Local transaction ID in the dependent domain, if applicable |
| `description` | `string` | Human-readable description of the specific violation |

### 16.3 PropagationChain

| Field | Type | Description |
|---|---|---|
| `root_cause` | `PropagationNode` | The earliest error in the causal chain |
| `dependents` | `PropagationNode[]` | Ordered list of dependent transactions |

**PropagationNode fields:** `domain_id`, `transaction_id` (local), `error_description`.

---

## 17. Reporting and Visualization

### 17.1 Multi-Protocol Report (schema_version `3.0`)

When Phase 7 is active with multiple domains, the report schema advances to version `3.0`. The report contains:

**`schema_version`** — `"3.0"`.

**`active_domains`** — An array of all domain ID strings that were active in this analysis session.

**`per_domain`** — An object with one key per active domain ID. Each value is that domain's full per-domain report section (schema v1.0 for PCIe, v2.0 if Phase 5 is also active, etc.).

**`cross_domain`** — An object containing:
- `correlations` — An array of `CrossDomainMatch` records.
- `violations` — An array of `CrossDomainViolation` records.
- `propagation_chains` — An array of `PropagationChain` records.

### 17.2 GUI Extensions (Phase 2 Integration)

When Phase 2 is present and multiple domains are active, the following additional views are available:

**Cross-domain transaction flow view** — A swimlane diagram with one horizontal lane per active domain. Each transaction is rendered as a bar in its domain's lane. Correlation matches are drawn as connecting lines between matched transactions in different lanes.

**Unified timeline view** — All events from all domains displayed on a single scrollable time axis. Events are color-coded by domain. Selecting an event highlights its correlated partners in other domains.

**Violation origin highlighting** — When a cross-domain violation is selected in the violation list, both the primary and the dependent transactions are highlighted simultaneously in their respective domain views.

---

## 18. Protocol Registration Guide

The following steps describe the complete procedure for integrating a new protocol domain into Traceon. No existing source file is modified by this procedure.

**Step 1 — Create the domain directory.** Create a new subdirectory under `src/domains/` with the protocol name (e.g., `src/domains/my_protocol/`).

**Step 2 — Implement the ProtocolDomain interface.** Create a class `MyProtocolDomain` that inherits from `ProtocolDomain` and implements all pure virtual methods. Place the implementation in the new domain directory.

**Step 3 — Implement the domain's analysis components.** Within the domain directory, implement a packet parser, a protocol validator, and a transaction mapper following the same structural pattern as the PCIe domain. These components are entirely internal to the domain and are not exposed through the `ProtocolDomain` interface except through the opaque handle types.

**Step 4 — Register the domain at startup.** In the platform configuration or in `main.cpp`, add a single line that registers the new domain with the `DomainRegistry`. This is the only change to any file outside the new domain directory.

**Step 5 — Define correlation rules (optional).** If the new domain has relationships with existing domains, add named correlation rule definitions to the `correlation_rules.json` configuration file. No C++ changes are required for correlation rules.

**Step 6 — Define address regions (optional).** If the new domain owns memory address ranges, add range definitions to the `address_map.json` configuration file.

**Step 7 — Add tests.** Add domain-specific unit tests in the domain directory. Add at minimum one cross-domain correlation system test involving the new domain and an existing domain.

The new domain is complete. No modification to any existing domain, the `CrossDomainLayer`, the `PlatformOrchestrator`, or any prior phase component is required.

---

## 19. Testing Strategy

### 19.1 Single-Protocol Regression

For every canonical Phase 1 trace file, run analysis in multi-protocol mode with only the PCIe domain registered. Confirm that the per-domain PCIe output is byte-identical to the corresponding Phase 1 baseline. This test must pass before any multi-domain feature is declared complete.

### 19.2 Dual-Protocol Scenarios

| Test Scenario | Domains Active | Expected Outcome |
|---|---|---|
| NVMe command → PCIe doorbell write correlation | nvme + pcie | `CORR-001` match produced; zero cross-domain violations |
| PCIe transaction accessing CXL memory without permission | pcie + cxl | `XDOM-002` violation reported with correct domain and address |
| NVMe command with no matching PCIe doorbell write | nvme + pcie | `XDOM-003` violation for the unmatched NVMe command |
| PCIe UR completion followed by NVMe command failure | pcie + nvme | Propagation chain in report linking PCIe UR to NVMe INCOMPLETE |
| Temporal ordering violation | pcie + nvme | `XDOM-004` detected when completion ordering contradicts logical dependency |

### 19.3 Multi-Domain Stress Tests

A three-domain simultaneous stress test with 10,000 transactions per domain (30,000 total) must complete correlation within the NFR-MULTI-04 time bound. Per-domain analysis quality for each domain must be indistinguishable from its single-domain baseline.

### 19.4 Extensibility Test

A fourth protocol domain shall be created by following the registration procedure in Section 18 with zero changes to any existing file. After registration, the platform must discover and analyze the new domain correctly without modification to any prior code.

---

## 20. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation Strategy |
|---|---|---|---|
| Correlation engine exhibits O(N²) behavior for large transaction sets, missing the NFR-MULTI-04 time bound | Medium | High | Enforce indexed lookup for all correlation rule evaluations. Benchmark correlation against the 30,000-transaction stress test before release. Any O(N²) implementation is a blocking defect. |
| A new domain implementation accidentally accesses another domain's internals through a shared registry pointer | Low | High | Domain isolation is enforced structurally: `DomainRegistry` holds `shared_ptr<ProtocolDomain>` only — the abstract interface. No domain exposes internal state through this interface. Code review must verify no unsafe downcasting. |
| Schema v3.0 report is not backward compatible with the Phase 2 GUI | Medium | Medium | Phase 2 `ReportParser` checks `schema_version` before loading. For version `3.0`, the parser processes the `per_domain.pcie` section using the existing Phase 1/2 logic and ignores the `cross_domain` section gracefully. |
| Overly broad correlation rules produce false-positive cross-domain matches | Medium | Medium | Provide a `--validate-rules` CLI option that loads and syntax-checks all correlation rules and reports potential over-breadth warnings without executing analysis. Use confidence scores to allow callers to filter low-confidence matches. |
| Domain-local transaction IDs collide when merged in cross-domain records | Low | Medium | All cross-domain data structures use a composite key of `(domain_id, local_txn_id)`. No raw `local_txn_id` integer is used outside its owning domain. |

---

## 21. Exit Criteria

Phase 7 is considered complete and accepted when all of the following conditions are simultaneously satisfied:

- [ ] The PCIe domain correctly implements the `ProtocolDomain` interface with zero modifications to Phase 1 source files, verified by code review and full Phase 1 regression test suite.
- [ ] The CXL domain is registered and produces correct per-domain analysis output for a sample CXL trace.
- [ ] The NVMe domain is registered and `CORR-001` correctly correlates NVMe write commands to PCIe doorbell writes for the NVMe-PCIe test scenario.
- [ ] `XDOM-002` (illegal cross-domain memory access) is detected and reported correctly with the correct primary domain and address information.
- [ ] Error propagation chains are produced and correctly structured for at least one verified PCIe error → NVMe failure scenario.
- [ ] Single-protocol PCIe regression tests pass with zero output differences.
- [ ] Cross-domain correlation completes within the NFR-MULTI-04 time bound for the 30,000-transaction stress test.
- [ ] Schema v3.0 report is valid and loadable by the Phase 2 GUI without error on the `per_domain.pcie` section.
- [ ] A fourth protocol domain can be added following the registration procedure in Section 18 with zero changes to existing domain or cross-domain source files.
- [ ] All unit, integration, dual-protocol, and stress tests pass with zero failures.


