# **PHASE 1 — Core PCIe Protocol Analyzer**

**(Standalone, Headless, Fully Functional Product)**

---

## 1. Phase 1 Objective

The objective of Phase 1 is to deliver a **reliable, offline PCIe protocol analysis engine** that can:

- Read PCIe trace data from files
- Decode transaction-level packets (TLPs)
- Validate protocol correctness using rule-based checks
- Produce professional, machine-readable reports
- Run deterministically on Linux

This phase establishes the **technical foundation** of the entire system and is considered a **complete product** on its own.

---

## 2. Phase 1 Scope Definition

### Included Capabilities

- Offline trace ingestion
- PCIe TLP decoding
- Transaction tracking and validation
- Error classification and reporting
- Command-line execution
- Unit and system-level testing
- Structured documentation

### Explicitly Excluded

- GUI or visualization
- Multithreading or concurrency
- Live streaming
- Scripting (Python/Tcl)
- SystemC / TLM
- Hardware-assisted verification

These exclusions are intentional to maintain clarity, stability, and correctness.

---

## 3. Functional Requirements

### 3.1 Trace Ingestion

- Accept trace files in a defined input format
- Read packets sequentially
- Preserve packet order and timestamps (if available)

### 3.2 Packet Decoding

- Decode raw data into structured PCIe TLP representations
- Support core PCIe transaction types:

  - Memory Read
  - Memory Write
  - Completion
  - Completion with Data

- Extract mandatory header fields

### 3.3 Protocol Validation

The system must detect, at minimum:

- Missing completions for read requests
- Duplicate or unexpected completions
- Invalid address alignment
- Invalid payload length
- Unsupported or malformed TLPs

Validation must be:

- Deterministic
- Rule-based
- Repeatable across runs

### 3.4 Reporting

- Generate a final analysis report
- Include:

  - Summary statistics
  - List of detected violations
  - Packet indices and descriptions

- Output format: JSON or XML

### 3.5 User Interaction

- Operated via command-line interface
- Accepts input/output parameters
- No interactive UI

---

## 4. Non-Functional Requirements

### 4.1 Reliability

- Analyzer must never crash on malformed input
- Errors must be reported, not cause termination

### 4.2 Maintainability

- Clear separation of responsibilities
- Easy to extend validation rules
- Clear documentation

### 4.3 Performance

- Able to process large trace files efficiently
- Single-threaded but optimized

### 4.4 Portability

- Linux-first design
- No platform-specific dependencies

---

## 5. Architectural Design

### 5.1 Logical Architecture

```
Trace File
   ↓
Trace Input Layer
   ↓
Packet Decode Layer
   ↓
Protocol Validation Layer
   ↓
Reporting Layer
   ↓
Analysis Report
```

Each layer is **strictly isolated** and communicates through well-defined data structures.

---

## 6. Component Responsibilities

### 6.1 Trace Input Layer

**Responsibility**

- File access and raw data extraction

**Constraints**

- No protocol awareness
- No validation logic

**Rationale**
Keeps I/O concerns isolated and reusable.

---

### 6.2 Packet Decode Layer

**Responsibility**

- Convert raw data into structured PCIe TLP objects
- Normalize packet representation

**Constraints**

- No validation
- No state tracking

**Rationale**
Decoding must remain deterministic and side-effect free.

---

### 6.3 Protocol Validation Layer

**Responsibility**

- Enforce PCIe correctness rules
- Track outstanding transactions
- Detect violations

**State Management**

- Maintain internal tables for:

  - Outstanding requests
  - Tags and requester IDs

**Rationale**
This layer embodies protocol expertise and system-level reasoning.

---

### 6.4 Reporting Layer

**Responsibility**

- Aggregate analysis results
- Produce structured output

**Constraints**

- No protocol logic
- No input parsing

**Rationale**
Decouples analysis from presentation and enables future integrations.

---

## 7. Data Flow Description

1. Trace file is opened and read sequentially
2. Each raw packet is decoded into a structured TLP
3. TLPs are passed to the validation engine
4. Validation engine updates internal state and records errors
5. After processing completes, a report is generated

This flow guarantees **full trace coverage** and **deterministic results**.

---

## 8. Error Model

### Error Categories

- Structural errors (malformed packets)
- Protocol rule violations
- Transaction consistency errors

### Error Metadata

Each error must include:

- Packet index
- Error category
- Rule name
- Human-readable description

---

## 9. Testing Strategy

### Unit Testing

- Packet decoding correctness
- Individual validation rules

### System Testing

- Valid traces → zero errors
- Invalid traces → predictable violations

### Regression Testing

- Re-run known traces after changes
- Ensure no behavior regression

---

## 10. Deliverables for Phase 1

- Executable analyzer
- Sample trace files
- Sample reports
- Design document (this phase)
- User guide (CLI usage)
- Test results

---

## 11. Phase 1 Exit Criteria

Phase 1 is considered complete when:

- Analyzer processes full traces without failure
- Validation rules produce correct, repeatable results
- Reports are structured and readable
- Tests pass consistently
- Documentation is complete and clear

At this point, Phase 1 is a **valid standalone product**.
