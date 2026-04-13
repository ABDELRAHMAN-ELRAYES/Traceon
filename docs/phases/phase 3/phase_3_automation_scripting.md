# Phase 3 — Automation and Scripting Layer

**Document ID:** PCIE-SPEC-PHASE-03  
**Project:** Traceon  
**Version:** 2.0.0  
**Status:** Approved  
**Phase Dependencies:** Phase 1 (PCIE-SPEC-PHASE-01) — scripts invoke the Phase 1 CLI as a subprocess and consume Phase 1 report files as their primary data source. Phase 2 is optional and is not required for Phase 3 functionality.  
**Consumed By:** Phase 4 (batch execution over live-capture runs), Phase 6 (automated fault coverage testing).

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Glossary](#2-glossary)
3. [Phase Independence Guarantee](#3-phase-independence-guarantee)
4. [Functional Requirements](#4-functional-requirements)
5. [Non-Functional Requirements](#5-non-functional-requirements)
6. [Architectural Design](#6-architectural-design)
7. [Script Interface Specification](#7-script-interface-specification)
8. [Python API Reference](#8-python-api-reference)
9. [Tcl API Reference](#9-tcl-api-reference)
10. [Report Post-Processing Model](#10-report-post-processing-model)
11. [Error Handling Strategy](#11-error-handling-strategy)
12. [Testing Strategy](#12-testing-strategy)
13. [Risks and Mitigations](#13-risks-and-mitigations)
14. [Exit Criteria](#14-exit-criteria)

---

## 1. Purpose and Scope

### 1.1 Purpose

Phase 3 introduces a **controlled automation and scripting layer** that enables engineers and CI systems to programmatically orchestrate analysis workflows, execute the Phase 1 analyzer across collections of trace files, validate results against defined expectations, and accumulate findings from multiple runs into consolidated reports. It surfaces the power of the Phase 1 engine to scripting environments without exposing, duplicating, or modifying any of its internals.

### 1.2 Objectives

- Provide Python and Tcl interfaces that invoke the Phase 1 CLI deterministically as a managed subprocess and reliably collect its standard output, standard error, and exit code.
- Enable batch execution across arbitrarily large collections of trace files, with per-file result isolation and resilience against individual trace failures.
- Support regression testing through a typed assertion model that evaluates report contents against expected values and emits actionable failure messages when expectations are not met.
- Allow the capture of analysis results as baseline files and the subsequent comparison of new results against those baselines to detect behavioral regressions.
- Produce consolidated summary reports aggregating statistics and error distributions across multiple analysis runs.
- Require zero modifications to Phase 1 or Phase 2 components to add or extend any Phase 3 capability.

### 1.3 In-Scope Capabilities

| Capability | Description |
|---|---|
| Script-driven single execution | Invoke the Phase 1 analyzer with configurable parameters from Python or Tcl |
| Batch processing | Execute the analyzer sequentially over a list of trace files and collect all results |
| Result assertion | Assert expected properties of a report and raise typed failures with descriptive messages |
| Baseline management | Save a run result as a baseline and compare future results against it |
| Regression suite execution | Execute a named suite of (trace, baseline) pairs and report pass/fail status |
| Report aggregation | Combine multiple run results into a single consolidated summary document |
| Post-processing utilities | Parse and query report JSON/XML from scripts through a typed model |

### 1.4 Explicitly Out of Scope

| Excluded Capability | Rationale |
|---|---|
| Live traffic injection or capture | Requires the Phase 4 concurrent infrastructure |
| GUI automation | Phase 2 is not a scripting target; window and mouse automation is out of scope |
| Concurrent batch execution | Parallel execution is introduced in Phase 4; Phase 3 batch is strictly sequential |
| Protocol logic within scripts | All analysis is performed by Phase 1; scripts read, assert, and aggregate only |
| Hardware interaction of any kind | Outside the scope of the software analyzer platform |

---

## 2. Glossary

| Term | Definition |
|---|---|
| **Script** | A Python or Tcl file that uses the Phase 3 API to drive one or more analysis workflows. |
| **Runner** | The Phase 3 API object responsible for managing the lifecycle of a Phase 1 analyzer subprocess. |
| **RunResult** | A typed object returned by the Runner after a single analyzer invocation, containing the exit code, output streams, parsed report model, and success flag. |
| **Batch run** | A single script invocation that executes the analyzer against a list of trace files, returning one `RunResult` per trace. |
| **Baseline** | A previously captured `RunResult` serialized to a JSON file and used as the reference expectation for regression comparison. |
| **Regression** | The act of comparing a current run result against a saved baseline to determine whether the analysis behavior has changed. |
| **RegressionResult** | A typed object describing whether a regression comparison passed, and if not, which fields changed and by how much. |
| **Assertion** | A script-level check that evaluates a specific property of a `RunResult` and raises a typed failure with a descriptive message if the property does not match the expected value. |
| **ConsolidatedReport** | A document aggregating statistics, error counts, and per-rule frequencies across a collection of `RunResult` objects. |
| **ReportModel** | A typed Python or Tcl data structure representing the parsed contents of a Phase 1 report, providing named accessors for all fields. |

---

## 3. Phase Independence Guarantee

Phase 3 is an **external control layer** that sits above Phase 1 and Phase 2 without modifying either. The following constraints are binding:

**Rule 1 — Subprocess-only invocation.** Scripts invoke the Phase 1 CLI as a subprocess. They do not link against Phase 1 libraries, call Phase 1 internal functions, or access Phase 1 data structures directly through memory. The CLI is the sole and complete interface.

**Rule 2 — No modification of prior phases.** No Phase 3 component, script module, or utility function requires any change to any Phase 1 or Phase 2 source file.

**Rule 3 — No protocol logic in scripts.** Scripts read fields from the `ReportModel`, assert counts and identifiers, and aggregate numbers. They must not perform bit-level computation, re-implement decoding logic, or derive PCIe protocol conclusions from raw packet data.

**Rule 4 — Removability without impact.** Removing all Phase 3 components must leave Phase 1 and Phase 2 fully functional and unaffected.

**Rule 5 — Versioned API.** The Phase 3 Python and Tcl APIs are independently versioned. Any breaking change to a public API function — change in signature, change in return type, or change in exception behavior — must increment the API version and be documented in the revision history.

---

## 4. Functional Requirements

### 4.1 Script-Driven Execution

| ID | Requirement |
|---|---|
| FR-SCR-01 | The Phase 3 API shall provide a function to invoke the Phase 1 analyzer with a specified input trace path and output report path, returning a `RunResult`. |
| FR-SCR-02 | The API shall allow the caller to specify the output report format as JSON or XML for each invocation. |
| FR-SCR-03 | The API shall capture the full standard output stream, the full standard error stream, and the integer exit code of the analyzer process and make all three available on the returned `RunResult`. |
| FR-SCR-04 | If the analyzer executable cannot be found at the configured path, the API shall raise a typed `AnalyzerNotFoundError` before attempting to start the process. |
| FR-SCR-05 | The API shall support a configurable per-invocation timeout expressed in seconds. If the analyzer process has not exited within the timeout period, the process shall be terminated and an `AnalyzerTimeoutError` shall be raised with the trace file path identified in the message. |
| FR-SCR-06 | By default, the run function is synchronous and blocking: the caller does not proceed until the analyzer subprocess has exited and the `RunResult` is fully populated. |

### 4.2 Batch Processing

| ID | Requirement |
|---|---|
| FR-BATCH-01 | The API shall provide a batch execution function that accepts a list of trace file paths and a target output directory, invoking the analyzer once per trace file. |
| FR-BATCH-02 | A failure on any individual trace file — including analyzer timeout, non-zero exit code, or report parse failure — must not interrupt the processing of subsequent trace files in the batch. All failures are captured in the corresponding `RunResult`. |
| FR-BATCH-03 | The batch function shall return a list of `RunResult` objects, one per trace file, in the same order as the input list. |
| FR-BATCH-04 | Batch execution in Phase 3 is strictly sequential. Parallel execution across trace files is not supported in this phase and is deferred to Phase 4. |
| FR-BATCH-05 | The batch function shall provide progress feedback to the caller after each trace file is processed, through either a caller-supplied callback function or an iterable interface that yields results incrementally. |

### 4.3 Result Assertion

| ID | Requirement |
|---|---|
| FR-ASSERT-01 | The API shall provide assertion functions that evaluate properties of a `RunResult` and raise a typed `AssertionError` in Python or return a non-zero code in Tcl when the asserted condition is not satisfied. |
| FR-ASSERT-02 | The following assertions shall be provided: total packet count equals a specified value; decode error count equals a specified value; validation error count equals a specified value; a specific `rule_id` string is present in the validation errors list; a specific `rule_id` string is absent from the validation errors list; the count of errors for a specific category equals a specified value. |
| FR-ASSERT-03 | Every failed assertion shall include a message that identifies the name of the assertion, the expected value, the actual value found, and the trace file path that was being evaluated. |
| FR-ASSERT-04 | All assertions shall operate exclusively on a parsed `ReportModel` object. Assertions that take raw JSON string input or perform string matching on the report text are not permitted. |

### 4.4 Regression Testing

| ID | Requirement |
|---|---|
| FR-REG-01 | The API shall provide a function to serialize a `RunResult` as a baseline JSON file to a specified path, establishing it as the reference expectation for future comparisons. |
| FR-REG-02 | The API shall provide a function to compare a current `RunResult` against a saved baseline file and return a `RegressionResult` object. |
| FR-REG-03 | The `RegressionResult` shall specify: whether the comparison passed; and for each field that differs, the field name, the expected value from the baseline, and the actual value from the current run. |
| FR-REG-04 | Regression comparison shall be insensitive to the `generated_at` timestamp field. Differences in this field shall not cause a comparison to fail. |
| FR-REG-05 | The API shall provide a suite runner function that accepts a list of `(trace_path, baseline_path)` pairs, executes the analyzer for each pair, compares the result against the corresponding baseline, and returns a consolidated pass/fail summary across the entire suite. |

### 4.5 Report Aggregation

| ID | Requirement |
|---|---|
| FR-AGG-01 | The API shall provide an aggregation function that accepts a list of `RunResult` objects and produces a `ConsolidatedReport`. |
| FR-AGG-02 | The `ConsolidatedReport` shall contain: total number of traces analyzed; total packet count across all runs; total decode error count across all runs; total validation error count across all runs; a per-rule frequency table listing how many times each `rule_id` was triggered across all runs; and a list of trace file paths for which at least one error was detected. |
| FR-AGG-03 | The `ConsolidatedReport` shall be serializable to a JSON file through an explicit `write(path)` operation. |

---

## 5. Non-Functional Requirements

| ID | Category | Requirement |
|---|---|---|
| NFR-SCR-01 | Safety | Scripts must not modify existing trace files or Phase 1 output files in-place. All output produced by Phase 3 is written to new files in caller-specified directories. Overwriting an input is never acceptable. |
| NFR-SCR-02 | Reliability | A script that terminates due to an unhandled exception must not corrupt, truncate, or delete any output files that were successfully written before the failure. |
| NFR-SCR-03 | Portability | The Python API must support Python 3.9 and all later Python 3.x versions. No third-party packages beyond the Python standard library shall be required for any Phase 3 functionality. |
| NFR-SCR-04 | Portability | The Tcl API must support Tcl 8.6 and all later versions. It shall depend only on the standard Tcl `exec` command and the standard `json` package. |
| NFR-SCR-05 | Stability | The public interface of each API module — function signatures, argument names, return types, and exception types — shall be stable. Internal refactoring must not break any caller script that conforms to the published API. |
| NFR-SCR-06 | Testability | Every public API function shall be covered by at minimum one unit test and one integration test, exercising both the success path and the primary failure path. |

---

## 6. Architectural Design

### 6.1 Automation Architecture

Phase 3 introduces no new analysis components. Its architecture is entirely concerned with orchestrating existing tools and data.

```
┌───────────────────────────────────────────────────────┐
│               Script (Python or Tcl)                  │
│   Defines workflow: which traces, what assertions,    │
│   what baselines, what output directory               │
├───────────────────────────────────────────────────────┤
│       Phase 3 API (pcie_automation / pcie package)    │
│   Runner, BatchRunner, Asserter, RegressionRunner,    │
│   Aggregator, ReportModel, ConsolidatedReport         │
├───────────────────────────────────────────────────────┤
│       Phase 1 CLI (pcie_analyzer subprocess)          │
│   Invoked by the Runner; produces report files        │
└───────────────────────────────────────────────────────┘
```

### 6.2 Subprocess Isolation

The `Runner` component starts the Phase 1 analyzer as an independent subprocess using the platform's standard process creation API. It does not share memory with the subprocess, does not inject any code into it, and does not intercept its internal operation. The only communication channel between the runner and the analyzer is the standard input/output streams, the exit code, and the report file written to the output path. This ensures that a crash or timeout in the analyzer process cannot destabilize the script or any other concurrent run in a batch.

### 6.3 Report Model as the Script Boundary

Scripts access analysis results exclusively through the `ReportModel` abstraction. The `ReportModel` is constructed by parsing the report file produced by the analyzer subprocess. It provides typed accessors for every section and field of the Phase 1 report schema. Direct string manipulation of raw JSON or XML content is prohibited in production scripts, as it bypasses schema validation and type safety.

### 6.4 Baseline File Format

Baseline files are serialized `ReportModel` objects written as JSON. They use the same schema as the Phase 1 report, with the `generated_at` field set to a canonical placeholder value so that it does not contribute to regression differences. Baselines are intended to be committed to version control alongside the trace files they correspond to.

---

## 7. Script Interface Specification

### 7.1 Execution Flow Contract

Every script-driven analysis workflow follows this canonical sequence:

1. Construct a `Runner` with the path to the `pcie_analyzer` executable.
2. Call `Runner.run(trace_path, output_dir)` or `Runner.run_batch(trace_list, output_dir)`.
3. Inspect each `RunResult` for success. If `success` is `False`, the `error` field explains the failure.
4. Access the parsed report through `RunResult.report`, which is a fully typed `ReportModel`.
5. Call assertion functions or regression comparison functions on the `ReportModel` as needed.
6. Optionally pass a list of results to the `aggregate()` function to produce a `ConsolidatedReport`.

### 7.2 RunResult Fields

| Field | Type | Description |
|---|---|---|
| `success` | `bool` | `True` if the analyzer exited with code 0 and the report was successfully parsed |
| `exit_code` | `int` | The integer exit code returned by the analyzer subprocess |
| `stdout` | `str` | Full captured standard output from the analyzer |
| `stderr` | `str` | Full captured standard error from the analyzer |
| `trace_path` | `str` | The trace file path that was analyzed in this run |
| `report_path` | `str` | The path where the output report was written |
| `report` | `ReportModel` | The parsed report, populated on success; `None` on failure |
| `error` | `str` | A human-readable description of what went wrong; empty string on success |

---

## 8. Python API Reference

### 8.1 Module: `pcie_automation.runner`

**`Runner(analyzer_path: str)`**
Constructs a new Runner pointing to the Phase 1 analyzer executable at the given path. Raises `AnalyzerNotFoundError` immediately at construction time if the path does not resolve to an executable file.

**`Runner.run(trace_path, output_dir, format="json", timeout_s=60) → RunResult`**
Invokes the analyzer synchronously on the given trace file, writing the report to a file in `output_dir`. Returns a `RunResult`. If the analyzer cannot be started, raises `AnalyzerNotFoundError`. If the timeout expires before the analyzer exits, raises `AnalyzerTimeoutError` and terminates the process.

**`Runner.run_batch(trace_paths, output_dir, format="json", timeout_s=60, progress_callback=None) → list[RunResult]`**
Invokes the analyzer once for each trace path in the list, in order. Failures on individual traces are captured in the corresponding `RunResult` and do not interrupt the batch. If `progress_callback` is supplied, it is called after each trace completes with the current index and the `RunResult`.

### 8.2 Module: `pcie_automation.asserter`

**`ReportAsserter(result: RunResult)`**
Constructs an asserter bound to a specific `RunResult`. All assertion methods operate on the `ReportModel` within that result.

**`assert_total_packets(expected: int)`**
Asserts that `summary.total_packets` equals `expected`. On failure, raises `AssertionError` with a message including the expected and actual values and the trace path.

**`assert_decode_error_count(expected: int)`**
Asserts that `summary.malformed_packet_count` equals `expected`.

**`assert_validation_error_count(expected: int)`**
Asserts that `summary.validation_error_count` equals `expected`.

**`assert_rule_present(rule_id: str)`**
Asserts that at least one `ValidationError` in the report carries the given `rule_id`.

**`assert_rule_absent(rule_id: str)`**
Asserts that no `ValidationError` in the report carries the given `rule_id`.

**`assert_category_count(category: str, expected: int)`**
Asserts that exactly `expected` validation errors belong to the given category.

### 8.3 Module: `pcie_automation.regression`

**`RegressionRunner(runner: Runner)`**
Constructs a regression runner that uses the provided `Runner` for subprocess execution.

**`RegressionRunner.save_baseline(result: RunResult, baseline_path: str)`**
Serializes the given `RunResult` to a baseline JSON file at `baseline_path`.

**`RegressionRunner.compare(result: RunResult, baseline_path: str) → RegressionResult`**
Compares the given result against the baseline at `baseline_path`. Returns a `RegressionResult` with `passed` flag and a list of `FieldChange` objects for any differing fields.

**`RegressionRunner.run_suite(pairs, output_dir) → SuiteResult`**
Accepts a list of `(trace_path, baseline_path)` tuples. Runs the analyzer for each trace, compares against the corresponding baseline, and returns a `SuiteResult` containing an `all_passed` flag and the list of individual `RegressionResult` objects.

### 8.4 Module: `pcie_automation.aggregator`

**`aggregate(results: list[RunResult]) → ConsolidatedReport`**
Combines the summary data from all provided results into a single `ConsolidatedReport` object. Handles empty lists and mixed success/failure results gracefully.

**`ConsolidatedReport.write(path: str)`**
Serializes the consolidated report to JSON at the given path.

---

## 9. Tcl API Reference

### 9.1 Package: `pcie`

The Tcl API provides the same capabilities as the Python API through Tcl procedures and dictionary-based return values.

**`pcie::set_analyzer_path {path}`**
Sets the path to the `pcie_analyzer` executable for all subsequent calls in the session. Must be called before any `pcie::run` invocations.

**`pcie::run {trace_path output_dir {format json} {timeout 60}} → dict`**
Invokes the analyzer as a subprocess. Returns a Tcl dictionary with the following keys: `success` (1 or 0), `exit_code`, `stdout`, `stderr`, `trace_path`, `report_path`, `report` (a nested dictionary of parsed report content, or empty string on failure), and `error`.

**`pcie::run_batch {trace_paths output_dir} → list`**
Runs the analyzer for each path in `trace_paths` and returns a list of result dictionaries.

**`pcie::assert_total_packets {result expected}`**
Returns 0 and prints a descriptive message on failure; returns 1 on success.

**`pcie::assert_rule_present {result rule_id}`**
Returns 0 and prints a descriptive message if `rule_id` is not found in the validation errors; returns 1 otherwise.

**`pcie::compare_baseline {result baseline_path} → dict`**
Returns a dictionary with `passed` (1 or 0) and `changes` (a list of `{field expected actual}` triples).

---

## 10. Report Post-Processing Model

### 10.1 ReportModel Structure

Scripts access report data through a `ReportModel` with named typed properties rather than through dictionary key access or string parsing. The model mirrors the Phase 1 report schema.

| Accessor | Type | Corresponds To |
|---|---|---|
| `report.schema_version` | `str` | `schema_version` |
| `report.generated_at` | `str` | `generated_at` |
| `report.trace_file` | `str` | `trace_file` |
| `report.summary.total_packets` | `int` | `summary.total_packets` |
| `report.summary.tlp_type_distribution` | `dict[str, int]` | `summary.tlp_type_distribution` |
| `report.summary.malformed_packet_count` | `int` | `summary.malformed_packet_count` |
| `report.summary.validation_error_count` | `int` | `summary.validation_error_count` |
| `report.summary.skipped_line_count` | `int` | `summary.skipped_line_count` |
| `report.malformed_packets` | `list[MalformedPacketModel]` | `malformed_packets` array |
| `report.validation_errors` | `list[ValidationErrorModel]` | `validation_errors` array |
| `report.packets` | `list[PacketModel]` | `packets` array |

### 10.2 Policy on Direct JSON Manipulation

Scripts must use the `ReportModel` accessors for all data access. Direct string manipulation of raw JSON content — including substring searches, regular expression matching, or manual dictionary access by string key — is prohibited in production scripts. This policy exists because schema evolution can silently invalidate string-level assumptions. The `ReportModel` layer centralizes schema adaptation and protects all scripts from it simultaneously.

---

## 11. Error Handling Strategy

### 11.1 Python Exception Taxonomy

| Exception Type | Triggering Condition | Caller Recovery Guidance |
|---|---|---|
| `AnalyzerNotFoundError` | Executable path does not exist at invocation time | Verify the path configuration; abort the workflow |
| `AnalyzerTimeoutError` | Analyzer process did not exit within the timeout period | Log the trace path; in batch mode, continue to the next trace |
| `ReportParseError` | The output report file cannot be parsed as valid JSON or XML | Log; mark `RunResult.success = False`; continue batch |
| `AssertionError` | An assertion check was not satisfied | Propagate to the caller; message includes the expected and actual values |
| `BaselineNotFoundError` | The specified baseline file path does not exist | Log; mark the regression as failed; continue the suite |

### 11.2 Batch Failure Policy

In all batch execution contexts, a failure on any individual trace file must not stop the processing of subsequent files. All failures are recorded in the corresponding `RunResult` objects and are available for inspection when the batch function returns. Scripts that require all-or-nothing semantics must explicitly check the success status of every result after the batch completes and apply their own error policy.

---

## 12. Testing Strategy

### 12.1 Unit Tests

| Component Under Test | Required Test Cases |
|---|---|
| `Runner.run` | Successful invocation returning populated result; trace file not found; analyzer timeout fires and process is terminated; analyzer exits with non-zero code |
| `ReportAsserter` | Each of the six assertion types passing with matching value; each type failing with descriptive message carrying expected vs. actual |
| `RegressionRunner.compare` | Identical result passes comparison; changed packet count fails with field identified; `generated_at` change does not fail comparison |
| `aggregate` | Empty input list returns valid empty report; single result aggregated correctly; list with mixed success/failure results handled without error |
| Tcl `pcie::run` | Valid invocation returns correctly structured dictionary; failure returns non-success dictionary |

### 12.2 Integration Tests

| Test Name | Description | Expected Outcome |
|---|---|---|
| End-to-end single run | Full workflow: Runner → Phase 1 CLI → report file → assertion passes | `RunResult.success == True`; assertion raises no error |
| Batch over 10 traces | Sequential execution of 10 trace files | 10 `RunResult` objects returned; all successful; results in correct order |
| Regression suite pass | Suite run where all current outputs match saved baselines | `SuiteResult.all_passed == True` |
| Regression suite fail | One baseline is deliberately modified to introduce a mismatch | `SuiteResult.all_passed == False`; the modified field is identified in `RegressionResult.changes` |

---

## 13. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation Strategy |
|---|---|---|---|
| Phase 1 CLI argument names or semantics change and break all scripts | Low | High | Pin the Phase 3 API's subprocess invocation to a specific Phase 1 CLI version. When Phase 1 CLI changes, increment the Phase 3 API version and update integration accordingly. |
| Batch runs accumulate large output directories without the caller noticing | Medium | Low | Log each output file path to standard output as it is created. Provide a `dry_run=True` option that reports what would be executed without actually running the analyzer. |
| The Tcl `json` package is unavailable in certain embedded EDA environments | Medium | Medium | Document the dependency explicitly in the installation guide. Provide a pure-Tcl minimal JSON subset parser as a bundled fallback that activates when the standard package is not available. |
| Script authors embed protocol logic in assertion checks | Medium | Medium | Enforce a code review policy requiring that all assertion calls reference `ReportModel` accessors only and contain no bit manipulation or protocol inference. Provide code review guidance with examples of acceptable and unacceptable assertions. |

---

## 14. Exit Criteria

Phase 3 is considered complete and accepted when all of the following conditions are simultaneously satisfied:

- [ ] `Runner.run` successfully invokes the Phase 1 analyzer, captures stdout/stderr/exit code, parses the report, and returns a fully populated `RunResult`.
- [ ] `Runner.run_batch` processes a list of ten trace files sequentially, returns ten `RunResult` objects in the correct order, and correctly isolates failures to their respective results.
- [ ] All six assertion types in `ReportAsserter` produce correct pass behavior with matching inputs and correct failure messages with mismatching inputs.
- [ ] The regression suite correctly detects a deliberately introduced change in a run result and identifies the changed field accurately.
- [ ] The aggregator produces a correct `ConsolidatedReport` from a set of mixed success and failure results.
- [ ] The Tcl API reproduces equivalent execution behavior to the Python API for all corresponding operations.
- [ ] All unit tests and integration tests pass with zero failures.
- [ ] Phase 1 and Phase 2 components remain entirely unmodified and fully operational.
- [ ] The API reference documentation is complete and includes a working example for every public API function in both Python and Tcl.


