<div align="center">

<h1>Traceon — PCIe Protocol Analyzer</h1>

<p><em>A deterministic, multi-phase C++ analysis platform for PCIe protocol trace inspection,<br/>transaction modeling, fault injection, and cross-domain correlation.</em></p>

![C++](https://img.shields.io/badge/C++-17%2F20-00599C?style=flat-square&logo=cplusplus)
![Asio](https://img.shields.io/badge/Asio-Live_I%2FO-blue?style=flat-square)
![SystemC](https://img.shields.io/badge/SystemC-TLM_2.0-orange?style=flat-square)
![Python](https://img.shields.io/badge/Python-3.9+-3776AB?style=flat-square&logo=python&logoColor=white)
![Tcl](https://img.shields.io/badge/Tcl-8.6+-EF7B00?style=flat-square)
![Qt](https://img.shields.io/badge/Qt-LGPL_GUI-41CD52?style=flat-square&logo=qt&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)
![Status](https://img.shields.io/badge/Status-Approved-brightgreen?style=flat-square)

**[Overview](#overview) · [Architecture](#architecture) · [Phases](#phase-breakdown) · [Getting Started](#getting-started) · [CLI Reference](#cli-reference) · [Testing](#testing) · [Contributing](#contributing)**

</div>

---

## Overview

The **Traceon** analyzer is a professional-grade, offline and live C++ analysis engine for PCIe transaction layer traffic. Designed for hardware validation engineers, EDA developers, and system architects, it decodes raw hexadecimal trace packets into fully structured TLP objects, enforces protocol correctness through a deterministic rule engine, and produces machine-readable reports for downstream consumption.

The system is architected across **seven independent, additive phases**. Each phase is a complete, shippable deliverable that extends the prior phases through wrapping and consumption — never modification. The Phase 1 analysis core is the permanent, unmodifiable foundation that all subsequent layers build upon.

### Key Capabilities at a Glance

| Domain                | Capability                                                                           |
| --------------------- | ------------------------------------------------------------------------------------ |
| **Protocol Analysis** | TLP decoding (MRd, MWr, Cpl, CplD), 9 decode rules, 8 validation rules               |
| **Visualization**     | Desktop GUI with packet table, detail panel, error summary, statistics               |
| **Automation**        | Python + Tcl scripting APIs, batch execution, regression testing                     |
| **Concurrency**       | Asio TCP live ingestion, thread-safe MPSC queue, GUI event channel                   |
| **TLM**               | Transaction reconstruction, lifecycle tracking, latency analysis, virtual components |
| **Fault Injection**   | 16-fault taxonomy, coverage reporting, detection gap analysis                        |
| **Multi-Protocol**    | PCIe + CXL + NVMe cross-domain correlation, unified address space, global timeline   |

---

## Architecture

### Layered Phase Model

The system is structured as a strict dependency pyramid. Each phase is additive and independently removable. The Phase 1 core is never modified.

```
┌──────────────────────────────────────────────────────────────────┐
│           Phase 7 — Multi-Protocol & Cross-Domain                │
│   DomainRegistry · ProtocolDomain · CrossDomainLayer · CXL/NVMe │
├──────────────────────────────────────────────────────────────────┤
│            Phase 6 — Error Injection & Fault Modeling            │
│     FaultInjectionLayer · FaultLog · FaultCoverageAnalyzer       │
├──────────────────────────────────────────────────────────────────┤
│        Phase 5 — Transaction-Level Modeling (TLM)                │
│  TransactionMapper · VirtualComponentRegistry · SystemValidator  │
├────────────────────────────────┬─────────────────────────────────┤
│  Phase 4 — Concurrency &       │  Phase 3 — Automation &         │
│  Live Data (Asio TCP)          │  Scripting (Python + Tcl)       │
│  QueueManager · ConsumerLoop   │  Runner · ReportAsserter        │
│  AsioSession · GuiEventChannel │  RegressionRunner · Aggregator  │
├────────────────────────────────┴─────────────────────────────────┤
│                Phase 2 — GUI Visualization Layer                  │
│       PacketTable · DetailPanel · ErrorPanel · StatsPanel        │
├──────────────────────────────────────────────────────────────────┤
│            Phase 1 — Core PCIe Protocol Analyzer                 │
│    TraceInputLayer · PacketDecoder · ProtocolValidator           │
│              ReportBuilder · CLI (pcie_analyzer)                 │
└──────────────────────────────────────────────────────────────────┘
```

### Data Flow — Offline Mode

```
  trace.csv
      │
      ▼
 TraceInputLayer          Parses CSV lines → RawPacket{index, timestamp_ns,
      │                   direction, payload_hex}
      ▼
 PacketDecoder            Decodes DW0–DW3 → TLP{type, fmt, tc, attr,
      │                   requester_id, tag, address, length_dw, …}
      │                   Detects: DEC-001 … DEC-009
      ▼
 ProtocolValidator        Tracks outstanding requests by (requester_id, tag).
      │                   Detects: VAL-001 … VAL-008
      ▼
 ReportBuilder            Emits JSON / XML report (schema v1.0+)
      │
      ▼
  report.json
```

### Data Flow — Live Mode (Phase 4)

```
  TCP Sender
      │  (length-prefix framing)
      ▼
  AsioSession             async_read on Asio io_context
      │  RawPacket
      ▼
  QueueManager            Bounded MPSC queue. Backpressure via blocking push().
      │  RawPacket         Capacity: configurable (default 1024)
      ▼
  ConsumerLoop            Pulls packets, drives Phase 1 pipeline identically to
      │                   offline mode.
      ├──▶ GuiEventChannel  Lock-free SPSC ring buffer → GUI Thread (if Phase 2)
      ▼
  ReportBuilder
```

---

## Phase Breakdown

### Phase 1 — Core PCIe Protocol Analyzer

> **Document:** `PCIE-SPEC-PHASE-01` ·  
> **Status:** Approved ·  
> **Dependencies:** None

The foundational, headless analysis engine. Operates entirely from the command line on Linux with no external runtime dependencies. This phase is a complete, shippable product in its own right and is the permanent authoritative analysis core — **no subsequent phase may modify its source files**.

**Components:**

| Component           | Responsibility                                                            |
| ------------------- | ------------------------------------------------------------------------- |
| `TraceInputLayer`   | Reads CSV trace files sequentially; produces `RawPacket` objects          |
| `PacketDecoder`     | Stateless decoder; converts hex payloads to structured `TLP` objects      |
| `ProtocolValidator` | Stateful; tracks outstanding `(requester_id, tag)` pairs across the trace |
| `ReportBuilder`     | Serializes analysis results to JSON or XML (schema v1.0)                  |

**Supported TLP Types:** `MRd`, `MWr`, `Cpl`, `CplD`

**Decode Error Rules (DEC-001 – DEC-009):**

| Rule ID | Condition Detected                                             |
| ------- | -------------------------------------------------------------- |
| DEC-001 | Reserved `Fmt` bit pattern                                     |
| DEC-002 | Unknown `Type` field                                           |
| DEC-003 | Payload present when `Fmt` indicates none                      |
| DEC-004 | Payload absent when `Fmt` requires it                          |
| DEC-005 | `Length` field is zero                                         |
| DEC-006 | `Length` exceeds 1024 DW (PCIe maximum)                        |
| DEC-007 | Packet truncated — insufficient bytes for declared header size |
| DEC-008 | Reserved `TC` value                                            |
| DEC-009 | Address not naturally aligned to transfer size                 |

**Protocol Validation Rules (VAL-001 – VAL-008):**

| Rule ID | Category                | Condition Detected                                               |
| ------- | ----------------------- | ---------------------------------------------------------------- |
| VAL-001 | `UNEXPECTED_COMPLETION` | `CplD`/`Cpl` received with no matching outstanding `MRd`         |
| VAL-002 | `MISSING_COMPLETION`    | `MRd` has no corresponding `CplD` at end of trace                |
| VAL-003 | `DUPLICATE_COMPLETION`  | Second `CplD` received for an already-completed request          |
| VAL-004 | `BYTE_COUNT_MISMATCH`   | `CplD` byte count inconsistent with request length               |
| VAL-005 | `TAG_REUSE`             | Tag reused before previous transaction completes                 |
| VAL-006 | `REQUESTER_ID_MISMATCH` | Completion `requester_id` does not match the outstanding request |
| VAL-007 | `STATUS_ERROR`          | Completion status is UR (`001`) or CA (`100`)                    |
| VAL-008 | `ADDRESS_MISALIGNMENT`  | Memory address not naturally aligned                             |

**Non-Functional Requirements:**

- Single-threaded correctness is established before concurrency is introduced (Phase 4).
- Must process 100,000 packets in under 2 seconds on the reference system.
- Peak memory must not exceed 512 MB for any trace file.
- Deterministic: identical inputs always produce identical outputs.

---

### Phase 2 — GUI Visualization Layer

> **Document:** `PCIE-SPEC-PHASE-02` ·  
> **Status:** Approved ·  
> **Dependencies:** Phase 1

A desktop GUI application that loads Phase 1 JSON/XML reports and provides interactive, read-only inspection. The GUI contains **no protocol analysis logic** — it is a pure presentation consumer.

**Views:**

| View                    | Description                                                                                                  |
| ----------------------- | ------------------------------------------------------------------------------------------------------------ |
| **Packet Table**        | Sortable, filterable, virtual-scrolled table of all packets. One row per packet. Color-coded by error state. |
| **Detail Panel**        | Displays all TLP header fields for the selected packet. Null values rendered as `—`.                         |
| **Error Summary Panel** | Lists all validation errors grouped by category. Double-click navigates to the offending packet.             |
| **Statistics Panel**    | Aggregated counts and TLP type distribution chart from the report summary section.                           |

**Row Color Convention:**

| State                               | Visual Treatment      |
| ----------------------------------- | --------------------- |
| Normal, no errors                   | Default background    |
| Validation error reference          | Amber/yellow tint     |
| Malformed (decode error)            | Red tint              |
| Both malformed and validation error | Red tint (precedence) |
| Currently selected                  | System highlight      |

**Layout:**

```
┌──────────────────────────────────────────────────────┐
│  Menu: File  View  Help                              │
├──────────────────────────────────────────────────────┤
│  [Filter bar: Type ▾] [Direction ▾] [Errors only □]  │
├──────────────────────────────────────────────────────┤
│                                     │                 │
│         Packet Table (70%)          │  Detail Panel   │
│                                     │  (30%)          │
│                                     │                 │
├─────────────────────────┬───────────┴─────────────────┤
│   Error Summary Panel   │     Statistics Panel        │
│   (bottom-left, 50%)    │     (bottom-right, 50%)     │
└─────────────────────────┴─────────────────────────────┘
```

**Performance targets:** Load 1M-packet reports in under 3 seconds; maintain 60 fps scroll.

---

### Phase 3 — Automation and Scripting Layer

> **Document:** `PCIE-SPEC-PHASE-03` ·  
> **Status:** Approved ·  
> **Dependencies:** Phase 1

A controlled automation layer exposing Python 3.9+ and Tcl 8.6+ APIs for programmatic workflow execution. Scripts invoke the Phase 1 CLI as a subprocess — they contain **no protocol logic**.

**Python API Modules:**

| Module                       | Key Classes / Functions                                      |
| ---------------------------- | ------------------------------------------------------------ |
| `pcie_automation.runner`     | `Runner.run()`, `Runner.run_batch()`                         |
| `pcie_automation.assertions` | `ReportAsserter` — typed assertion methods                   |
| `pcie_automation.regression` | `RegressionRunner.compare()`, `RegressionRunner.run_suite()` |
| `pcie_automation.aggregator` | `aggregate(results)` → `ConsolidatedReport`                  |

**Regression Suite Example (Python):**

```python
from pcie_automation.runner import Runner
from pcie_automation.regression import RegressionRunner

runner = Runner("/usr/local/bin/pcie_analyzer")
reg    = RegressionRunner()

suite = [
    ("traces/canonical_valid.csv",      "baselines/canonical_valid_baseline.json"),
    ("traces/missing_completion.csv",   "baselines/missing_completion_baseline.json"),
]

result = reg.run_suite(suite, output_dir="/tmp/regression_output")
print(f"Regression: {'PASSED' if result.all_passed else 'FAILED'}")
```

**Batch Analysis with Aggregation:**

```python
from pcie_automation.runner import Runner
from pcie_automation.aggregator import aggregate
import glob

runner  = Runner("/usr/local/bin/pcie_analyzer")
results = runner.run_batch(glob.glob("/data/traces/*.csv"), "/data/reports")
report  = aggregate(results)
report.write("/data/reports/consolidated.json")
```

**Tcl API** mirrors the Python API semantics using `pcie::run`, `pcie::batch_run`, and `pcie::assert_*` commands. No third-party packages are required beyond the standard Tcl `json` package (with a pure-Tcl fallback provided).

---

### Phase 4 — Concurrency and Live Data Processing

> **Document:** `PCIE-SPEC-PHASE-04` ·  
> **Status:** Approved ·  
> **Dependencies:** Phase 1 (Phase 2 optional)

Extends the analyzer to support **real-time packet ingestion** over TCP using standalone Asio. Introduces a thread-safe producer–consumer pipeline. The Phase 1 offline path is **unchanged and remains the default**.

**Thread Model:**

| Thread               | Role                                                               |
| -------------------- | ------------------------------------------------------------------ |
| **Main Thread**      | Startup, configuration, shutdown coordination                      |
| **Ingestion Thread** | Runs `asio::io_context`; handles async TCP reads via `AsioSession` |
| **Consumer Thread**  | Pulls `RawPacket` from `QueueManager`; drives Phase 1 pipeline     |
| **GUI Thread**       | Polls `GuiEventChannel` at ~60 Hz (when Phase 2 is active)         |

**`QueueManager` Contract:**

- Bounded MPSC queue (default capacity: 1024 packets)
- `push()` blocks when full, providing implicit backpressure to the producer
- `pop()` blocks until a packet is available or `shutdown()` is called
- `shutdown()` unblocks all waiting callers atomically (no deadlock)

**Live Ingestion Protocol — Length-Prefix Framing:**

```
 ┌─────────────┬────────────────────────────────────────┐
 │  4 bytes    │  N bytes                               │
 │ std::uint32_t   │  RawPacket (JSON or binary)            │
 │  (N, LE)    │                                        │
 └─────────────┴────────────────────────────────────────┘
```

**CLI Extensions:**

```
pcie_analyzer --live --host 127.0.0.1 --port 9876 --queue-capacity 1024 --output report.json
```

**Invariant:** Offline mode and live mode produce **bit-identical reports** for the same packet sequence. This is verified in CI on every build.

---

### Phase 5 — Transaction-Level Modeling (TLM)

> **Document:** `PCIE-SPEC-PHASE-05` ·  
> **Status:** Approved ·  
> **Dependencies:** Phase 1, Phase 4 (optional)

Elevates analysis from packet-level observation to **logical transaction abstraction**. Groups related TLPs into `Transaction` objects, tracks their full lifecycle, computes latency, and detects system-level violations invisible at the individual-packet level.

**Transaction Lifecycle State Machine:**

```
              ┌─────────┐
              │ PENDING │
              └────┬────┘
      ┌────────────┼────────────┐
      ▼            ▼            ▼
 ┌──────────┐ ┌────────┐ ┌──────────┐
 │ COMPLETE │ │ ERROR  │ │ TIMEOUT  │
 └──────────┘ └────────┘ └──────────┘
```

- `COMPLETE`: CplD received with SC status and matching byte count
- `ERROR`: Completion received with UR (`001`) or CA (`100`) status
- `TIMEOUT`: No completion within the configurable `txn_timeout_ns` threshold
- `INCOMPLETE`: No completion by end of trace (below timeout threshold)

**System-Level Validation Rules (SYS-001 – SYS-005):**

| Rule ID | Condition                                                |
| ------- | -------------------------------------------------------- |
| SYS-001 | Transaction timeout — no completion within threshold     |
| SYS-002 | Completion status UR or CA                               |
| SYS-003 | Duplicate completion for an already-complete transaction |
| SYS-004 | Byte count returned does not match bytes requested       |
| SYS-005 | Initiator traffic class violation across transactions    |

**GUI Extensions (requires Phase 2):**

- **Transaction Timeline View:** Horizontal bars on a time axis, color-coded by state (green=COMPLETE, red=ERROR, amber=TIMEOUT, gray=INCOMPLETE).
- **Virtual Component Panel:** Inferred initiators and targets with per-device error rates.
- **Latency Distribution Chart:** Bar chart of latency buckets per transaction type with mean/min/max annotations.

**Report Schema:** Extends Phase 1 JSON to schema version `2.0`. Fully backward-compatible — consumers that read only the v1.0 fields are unaffected.

---

### Phase 6 — Error Injection and Fault Modeling

> **Document:** `PCIE-SPEC-PHASE-06` ·  
> **Status:** Approved ·  
> **Dependencies:** Phase 1, Phase 4 (optional)

A **verification overlay** that introduces a transparent `FaultInjectionLayer` before the `PacketDecoder`. Enables controlled negative-path testing without modifying any previously delivered component. All faults are deterministic, explicit, and observable.

**Fault Taxonomy (16 faults):**

| Class                       | Fault                                | Target Rule       |
| --------------------------- | ------------------------------------ | ----------------- |
| Structural (STRUCT-001–009) | `FIELD_OVERRIDE` (Fmt, Type, TC, …)  | DEC-001 – DEC-009 |
| Structural                  | `PACKET_TRUNCATION`                  | DEC-007           |
| Structural                  | `PAYLOAD_CORRUPTION` (byte/bit flip) | DEC-003 / DEC-004 |
| Protocol (PROTO-001–004)    | `COMPLETION_SUPPRESS`                | VAL-002           |
| Protocol                    | `COMPLETION_DUPLICATE`               | VAL-003           |
| Protocol                    | `STATUS_OVERRIDE` (UR/CA)            | VAL-007 / SYS-002 |
| Protocol                    | `BYTE_COUNT_OVERRIDE`                | VAL-004           |
| Timing (TIME-001–002)       | `TIMESTAMP_OVERRIDE`                 | SYS-001 (timeout) |
| Timing                      | `PACKET_REORDER`                     | VAL-008           |

**Fault Scenario Configuration (JSON):**

```json
{
  "scenario_id": "full_decode_coverage",
  "description": "Exercises all DEC-xxx rules once each.",
  "faults": [
    {
      "fault_id": "STRUCT-001",
      "type": "FIELD_OVERRIDE",
      "injection_point": { "type": "BY_INDEX", "packet_index": 0 },
      "field_name": "Fmt",
      "field_value": 6,
      "expected_rule": "DEC-001"
    }
  ]
}
```

**CLI:**

```
pcie_analyzer --input trace.csv --output report.json \
              --inject scenario.json --fault-report fault_report.json
```

**Fault Coverage Report** maps every injected fault to the rule it triggered (or flags it as a `DETECTION_GAP`). Any detection gap is a CI failure. When injection is disabled, the pipeline is byte-for-byte identical to Phases 1–5.

---

### Phase 7 — Multi-Protocol and Cross-Domain Modeling

> **Document:** `PCIE-SPEC-PHASE-07` ·  
> **Status:** Approved ·  
> **Dependencies:** Phase 1, Phase 5

The **terminal extension phase**. Transforms the analyzer into a multi-protocol platform by introducing a `ProtocolDomain` abstraction, a `DomainRegistry`, and a `CrossDomainLayer` that correlates transactions across protocol boundaries.

**Supported Protocol Domains:**

| Domain | Status                                                                          |
| ------ | ------------------------------------------------------------------------------- |
| `pcie` | Reference implementation (Phase 1)                                              |
| `cxl`  | Additive — implements `ProtocolDomain` interface                                |
| `nvme` | Additive — implements `ProtocolDomain` interface                                |
| Custom | Register via `DomainRegistry::register_domain()` — zero existing files modified |

**Cross-Domain Validation Rules (XDOM-001 – XDOM-005):**

| Rule ID  | Category                      | Condition                                                                                   |
| -------- | ----------------------------- | ------------------------------------------------------------------------------------------- |
| XDOM-001 | `CORRELATION_MISMATCH`        | Correlated transactions in different domains carry inconsistent address or size             |
| XDOM-002 | `ILLEGAL_CROSS_DOMAIN_ACCESS` | Transaction accesses an address range owned by a different domain without a defined mapping |
| XDOM-003 | `UNMATCHED_TRANSACTION`       | No counterpart found in the target domain for a transaction subject to a correlation rule   |
| XDOM-004 | `OUT_OF_ORDER_CROSS_DOMAIN`   | Correlated transactions violate expected ordering across domain boundaries                  |
| XDOM-005 | `ERROR_PROPAGATION`           | An error in a primary domain causes a dependent failure in a secondary domain               |

**Adding a New Protocol (zero existing files modified):**

```cpp
// Step 1: Implement the interface
class MyProtocolDomain : public ProtocolDomain { /* ... */ };

// Step 2: Register at startup
registry.register_domain(std::make_shared<MyProtocolDomain>());

// Step 3: (Optional) Define correlation rules in correlation_rules.json
// Step 4: (Optional) Define address regions in address_map.json
```

**Report Schema:** Extends to schema version `3.0`. Per-domain report sections are keyed by `domain_id`. Phase 2 GUI reads the per-domain sections it understands and gracefully ignores `cross_domain` content.

---

## Getting Started

### Prerequisites

| Dependency                      | Version            | Purpose                        |
| ------------------------------- | ------------------ | ------------------------------ |
| C++ Compiler (GCC / Clang)      | C++17 or C++20     | Core build                     |
| CMake                           | 3.20+              | Build system                   |
| Asio (standalone or Boost.Asio) | 1.18+              | Phase 4 live ingestion         |
| Qt (LGPL) or wxWidgets          | Qt 5.15+ / wx 3.2+ | Phase 2 GUI                    |
| Python                          | 3.9+               | Phase 3 scripting              |
| Tcl                             | 8.6+               | Phase 3 scripting              |
| ThreadSanitizer (TSan)          | GCC/Clang built-in | Phase 4 concurrency validation |

### Build — Phase 1 Only (Headless Analyzer)

```bash
git clone https://github.com/ABDELRAHMAN-ELRAYES/Traceon.git
cd Traceon
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target pcie_analyzer
```

### Build — All Phases

```bash
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DENABLE_GUI=ON \
  -DENABLE_SCRIPTING=ON \
  -DENABLE_LIVE=ON \
  -DENABLE_TLM=ON \
  -DENABLE_FAULT_INJECTION=ON \
  -DENABLE_MULTI_PROTOCOL=ON
cmake --build build
```

### Install Python Scripting API (Phase 3)

```bash
pip install -e scripting/python/
```

---

## CLI Reference

### Phase 1 — Core Analysis

```
pcie_analyzer --input <trace_file> --output <report_file> [options]
```

| Argument               | Required | Default | Description                         |
| ---------------------- | -------- | ------- | ----------------------------------- |
| `--input <path>`       | Yes      | —       | Input trace file (CSV format)       |
| `--output <path>`      | Yes      | —       | Output report file                  |
| `--format <json\|xml>` | No       | `json`  | Report output format                |
| `--verbose`            | No       | off     | Print per-packet progress to stdout |
| `--version`            | No       | —       | Print analyzer version and exit     |
| `--help`               | No       | —       | Print usage and exit                |

### Phase 4 — Live Mode Extensions

| Argument               | Default     | Description                                          |
| ---------------------- | ----------- | ---------------------------------------------------- |
| `--live`               | off         | Enable live TCP ingestion mode (overrides `--input`) |
| `--host <addr>`        | `127.0.0.1` | IP address to listen on                              |
| `--port <N>`           | `9876`      | TCP port to listen on                                |
| `--queue-capacity <N>` | `1024`      | `QueueManager` maximum packet count                  |

### Phase 6 — Fault Injection Extensions

| Argument                | Description                                              |
| ----------------------- | -------------------------------------------------------- |
| `--inject <path>`       | Path to fault scenario JSON file; enables injection mode |
| `--fault-report <path>` | Path for fault diagnostic report output                  |

### Phase 7 — Multi-Protocol Extensions

| Argument                     | Description                                       |
| ---------------------------- | ------------------------------------------------- |
| `--domain <id>:<trace_path>` | Register a domain and its trace file (repeatable) |
| `--address-map <path>`       | Unified address space configuration JSON          |
| `--correlation-rules <path>` | Cross-domain correlation rules JSON               |
| `--validate-rules`           | Check rule syntax without running analysis        |

### Exit Codes

| Code | Meaning                                |
| ---- | -------------------------------------- |
| `0`  | Analysis completed; report written     |
| `1`  | Input file could not be opened or read |
| `2`  | Output path could not be written       |
| `3`  | Invalid command-line arguments         |

> **Note:** The presence of decode or validation errors in the trace does **not** produce a non-zero exit code. The analyzer reports findings; it does not judge them. Non-zero codes are reserved for execution failures only.

---

## Input Format

The analyzer accepts **CSV trace files** with the following line format:

```
<timestamp_ns>,<direction>,<payload_hex>
```

| Field          | Description                                                                |
| -------------- | -------------------------------------------------------------------------- |
| `timestamp_ns` | 64-bit unsigned integer; use `0` if unavailable                            |
| `direction`    | `TX` or `RX`                                                               |
| `payload_hex`  | Space-separated hex bytes of the raw TLP (e.g., `40 00 00 01 00 00 00 0F`) |

Lines beginning with `#` are treated as comments and skipped. Malformed lines are logged as ingestion warnings and skipped; processing continues.

**Example:**

```csv
# PCIe capture — device enumeration
1000,TX,20 00 00 01 0A 01 00 0F 30 00 00 00
3100,RX,4A 00 00 01 00 00 10 0F 00 00 00 00 00 00 00 00 DE AD BE EF
```

---

## Output Report

### Schema v1.0 (Phase 1)

```json
{
  "schema_version": "1.0",
  "generated_at": "2025-04-13T10:00:00Z",
  "trace_file": "trace.csv",
  "summary": {
    "total_packets": 2,
    "tlp_type_distribution": { "MRd": 1, "CplD": 1 },
    "malformed_packet_count": 0,
    "validation_error_count": 0,
    "skipped_line_count": 0
  },
  "malformed_packets": [],
  "validation_errors": [],
  "packets": [
    {
      "index": 0,
      "timestamp_ns": 1000,
      "direction": "TX",
      "is_malformed": false,
      "tlp": {
        "type": "MRd",
        "header_fmt": "3DW",
        "tc": 0,
        "attr": { "no_snoop": false, "relaxed_ordering": false },
        "requester_id": "0000:05:01.0",
        "completer_id": null,
        "tag": 10,
        "address": "0x30000000",
        "length_dw": 4,
        "has_data": false,
        "byte_count": null,
        "status": null
      }
    }
  ]
}
```

### Schema Evolution

| Version | Phase   | New Sections Added                             |
| ------- | ------- | ---------------------------------------------- |
| `1.0`   | Phase 1 | Baseline report structure                      |
| `2.0`   | Phase 5 | `transactions`, `virtual_components`           |
| `3.0`   | Phase 7 | `active_domains`, `per_domain`, `cross_domain` |

All schema versions are backward-compatible. Consumers that ignore unknown keys remain fully functional across version upgrades.

---

## Testing

### Test Strategy by Phase

Each phase carries its own mandatory test suite. All tests must pass before a phase is marked accepted.

**Phase 1 — Unit Tests:**

| Component           | Coverage                                                                                |
| ------------------- | --------------------------------------------------------------------------------------- |
| `PacketDecoder`     | One test per DEC rule (DEC-001–DEC-009); one per TLP type; field extraction correctness |
| `ProtocolValidator` | One test per VAL rule (VAL-001–VAL-008); multi-packet tag tracking sequences            |
| `TraceInputLayer`   | Empty file, comment-only, mixed valid/malformed lines, no trailing newline              |
| `ReportBuilder`     | JSON and XML correctness; null field serialization; empty report                        |

**Phase 1 — System Tests:**

| Scenario                       | Expected Outcome                           |
| ------------------------------ | ------------------------------------------ |
| Canonical valid trace          | Zero decode errors, zero validation errors |
| All malformed packet types     | Each DEC rule triggered exactly once       |
| All validation error types     | Each VAL rule triggered exactly once       |
| Empty trace file               | Report with zero packets, zero errors      |
| Large trace (100,000+ packets) | Completes within the 2-second NFR bound    |

**Phase 4 — Concurrency Requirements:**

All unit and integration tests must be run under **ThreadSanitizer (TSan)**. Any TSan error is treated as a blocking defect and must be resolved before merge.

```bash
cmake -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build-tsan
ctest --test-dir build-tsan
```

**Phase 6 — Fault Coverage:**

```bash
pcie_analyzer --input traces/canonical.csv --output report.json \
              --inject scenarios/full_coverage.json \
              --fault-report coverage.json

# Expected: coverage_percent == 100.0, detection_gaps == 0
```

### Running All Tests

```bash
cmake --build build
ctest --test-dir build --output-on-failure
```

### Regression Baseline Update

```bash
python scripting/python/tools/update_baselines.py \
  --traces traces/canonical/ \
  --output baselines/
```

---

## Project Structure

```
Traceon/
├── src/
│   ├── backend/                 # Core analysis engine (Phase 1)
│   │   ├── include/backend/     # Namespaced public headers
│   │   │   ├── core/            # Packet and TLP definitions
│   │   │   └── decoder/         # Protocol decoding logic
│   │   └── src/                 # Implementation files
│   ├── gui/                     # Phase 2 — Visualization
│   │   ├── include/gui/
│   │   └── src/
│   ├── main.cpp                 # Application entry point
│   └── CMakeLists.txt
├── docs/
│   └── notes/                   # Phase specifications and notes
├── tests/
│   ├── unit/                    # Per-component unit tests
│   └── system/                  # End-to-end trace tests
├── data/                        # Sample trace files and logs
├── scripts/                     # Automation and helper scripts
├── CMakeLists.txt               # Root project configuration
└── README.md
```

---

## Phase Independence Guarantee

This invariant is binding on all contributors and enforced by CI:

1. **Phase 1 source files are read-only** after Phase 1 acceptance. No subsequent phase may modify them. A CI job fails if Phase 1 files are modified after the acceptance tag.
2. **Each phase must build and execute in isolation.** A build flag or configuration option must not be required to run Phase 1 independently.
3. **Disabling any phase returns the system to the behavior of all prior phases.** There are no hidden dependencies.
4. **Identical inputs always produce identical outputs**, regardless of which phases are enabled (determinism invariant).
5. **Report schema changes are versioned.** Each schema version increment maintains backward compatibility with all prior versions.

---

## Specification Documents

Full technical specifications for each phase are maintained in `docs/`:

| Document                                                                                                 | Phase   | Description                                                    |
| -------------------------------------------------------------------------------------------------------- | ------- | -------------------------------------------------------------- |
| [`phase_1_core_analyzer.md`](docs/phases/phase%201/phase_1_core_analyzer.md)                             | Phase 1 | TLP decoding, protocol validation, report format, CLI          |
| [`phase_2_gui_visualization.md`](docs/phases/phase%202/phase_2_gui_visualization.md)                     | Phase 2 | GUI views, view models, interaction model                      |
| [`phase_3_automation_scripting.md`](docs/phases/phase%203/phase_3_automation_scripting.md)               | Phase 3 | Python/Tcl APIs, batch execution, regression framework         |
| [`phase_4_concurrency_live_data.md`](docs/phases/phase%204/phase_4_concurrency_live_data.md)             | Phase 4 | Asio integration, thread model, queue design, framing protocol |
| [`phase_5_tlm_virtual_system.md`](docs/phases/phase%205/phase_5_tlm_virtual_system.md)                   | Phase 5 | Transaction model, lifecycle, system validation rules          |
| [`phase_6_error_injection.md`](docs/phases/phase%206/phase_6_error_injection.md)                         | Phase 6 | Fault taxonomy, scenario format, coverage analysis             |
| [`phase_7_multi_protocol_cross_domain.md`](docs/phases/phase%207/phase_7_multi_protocol_cross_domain.md) | Phase 7 | Protocol abstraction, domain registry, cross-domain rules      |

---

## Contributing

### Development Philosophy

- **Never modify a prior phase's source files.** Extend by wrapping or consuming.
- **No protocol logic in non-core components.** The GUI, scripting layer, and GUI event channel carry no PCIe knowledge.
- **All new rules must have a test.** One test per decode rule, one per validation rule, one per fault.
- **ThreadSanitizer must be green.** Any TSan error is a blocking defect.
- **Determinism is not negotiable.** If your change makes output non-deterministic under any input, it will not be merged.

### Adding a New Protocol Domain (Phase 7)

See [`docs/phases/phase 7/phase_7_multi_protocol_cross_domain.md`](docs/phases/phase%207/phase_7_multi_protocol_cross_domain.md), Section 18 — _Protocol Registration_. The process requires implementing one interface, registering one domain, and writing one system test. No existing file is modified.

### Adding a New Validation Rule

1. Add the rule definition to `docs/phase_1_core_pcie_analyzer.md` (Section 10).
2. Implement detection in `src/core/protocol_validator.cpp`.
3. Add one unit test in `tests/unit/test_protocol_validator.cpp`.
4. Add one system trace that triggers the rule in `traces/`.
5. Update the baseline in `baselines/`.
6. Increment the rule ID counter in the specification.

### Pull Request Checklist

- [ ] Phase 1 source files are unmodified (verified by CI diff check)
- [ ] All unit tests pass (`ctest --test-dir build`)
- [ ] TSan clean: zero data races (`ctest --test-dir build-tsan`)
- [ ] Offline equivalence test passes (if Phase 4 is involved)
- [ ] Fault injection disabled produces byte-identical output (if Phase 6 is involved)
- [ ] Single-protocol PCIe regression passes (if Phase 7 is involved)
- [ ] Specification document updated for any new rules, formats, or CLI arguments

---

## License

This project is released under the [MIT License](LICENSE).

---

<div align="center">

**PCIe Protocol Analyzer** — Built for hardware validation engineers who demand correctness.

_Phase 1 is always operational. Everything else is an extension._

</div>
