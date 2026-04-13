# Phase 2 — GUI Visualization Layer

**Document ID:** PCIE-SPEC-PHASE-02  
**Project:** Traceon  
**Version:** 2.0.0  
**Status:** Approved  
**Phase Dependencies:** Phase 1 (PCIE-SPEC-PHASE-01) — consumes the analysis report schema and the `TLP` / `ValidationError` data structures as defined therein.  
**Consumed By:** Phase 4 (live GUI update model), Phase 5 (transaction timeline view), Phase 7 (multi-protocol domain views).

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Glossary](#2-glossary)
3. [Phase Independence Guarantee](#3-phase-independence-guarantee)
4. [Functional Requirements](#4-functional-requirements)
5. [Non-Functional Requirements](#5-non-functional-requirements)
6. [Architectural Design](#6-architectural-design)
7. [Component Specifications](#7-component-specifications)
8. [View Model Definitions](#8-view-model-definitions)
9. [View Layout and Panel Specifications](#9-view-layout-and-panel-specifications)
10. [User Interaction Model](#10-user-interaction-model)
11. [Error Handling Strategy](#11-error-handling-strategy)
12. [Testing Strategy](#12-testing-strategy)
13. [Risks and Mitigations](#13-risks-and-mitigations)
14. [Exit Criteria](#14-exit-criteria)

---

## 1. Purpose and Scope

### 1.1 Purpose

Phase 2 introduces a **graphical visualization layer** to the Traceon platform. It provides engineers with an interactive desktop interface through which they can load, navigate, inspect, and reason about analysis results produced by the Phase 1 engine — without re-executing any analysis and without modifying any underlying data. The GUI is a presentation and inspection instrument; it adds no analytical logic to the platform and alters no data it displays.

### 1.2 Objectives

- Provide a desktop application that loads Phase 1 JSON or XML analysis reports from the filesystem and presents their contents interactively.
- Display decoded packets in a navigable, filterable, and sortable tabular view capable of rendering reports with up to one million packets without degrading responsiveness.
- Apply clear, consistent visual distinction to malformed packets and packets associated with protocol violations, ensuring that critical findings are immediately apparent.
- Enable per-packet inspection of all header fields, decode errors, and associated validation errors through a context-sensitive detail panel.
- Surface aggregated statistics from the report summary section in a dedicated statistics panel.
- Provide an error summary panel that enumerates all validation errors and allows direct navigation from any error to its originating packet.

### 1.3 In-Scope Capabilities

| Capability | Description |
|---|---|
| Report loading | Open and parse Phase 1 JSON or XML analysis reports from the local filesystem |
| Packet table | Scrollable, sortable, filterable tabular display of all packets in the loaded report |
| Error highlighting | Visual distinction between malformed packets, validation-error packets, and clean packets |
| Packet detail panel | Full per-field inspection of a selected packet including all associated errors |
| Statistics panel | Aggregated counts and TLP type distribution from the report summary section |
| Error summary panel | Categorized list of all validation errors with double-click navigation to the source packet |

### 1.4 Explicitly Out of Scope

| Excluded Capability | Rationale |
|---|---|
| Live or streaming data display | Requires the concurrent infrastructure introduced in Phase 4; not available in this phase |
| Re-analysis or re-decoding within the GUI | All analysis is performed by the Phase 1 engine; the GUI consumes results only |
| Editing or modifying trace data or report content | The GUI is a strictly read-only inspection tool |
| Scripting or automation of GUI actions | Phase 3 addresses automation at the CLI level; GUI automation is not in scope |
| SystemC or TLM transaction views | Phase 5 extensions |
| Multi-protocol domain views | Phase 7 extensions |

---

## 2. Glossary

| Term | Definition |
|---|---|
| **View Model** | A GUI-specific data representation derived from a Phase 1 report object. Contains only the fields needed for display, in a form optimized for the rendering engine. |
| **Packet Table** | The primary panel in the GUI layout — a scrollable, sortable table listing every packet from the loaded report. |
| **Detail Panel** | A context-sensitive side panel displaying all decoded fields and associated errors for the currently selected packet. |
| **Error Summary Panel** | A panel listing all validation and decode errors, grouped by category, with navigation links to the originating packets. |
| **Statistics Panel** | A panel presenting aggregated count and distribution data sourced from the report's summary section. |
| **Report** | A Phase 1 analysis report in JSON or XML format, conforming to the schema defined in PCIE-SPEC-PHASE-01, Section 12. |
| **Schema Version** | The `schema_version` field in the report header, used to verify format compatibility before loading. |
| **Selection** | The single packet row that is currently focused in the packet table and whose data is reflected in the detail panel. |
| **Virtual Rendering** | A technique for rendering large tabular datasets in which only the rows currently visible in the viewport are instantiated in the display hierarchy, allowing arbitrarily large datasets to scroll without memory exhaustion. |
| **View Model Derivation** | The one-time transformation step at report load time that converts raw report JSON/XML into typed view model objects optimized for display. |

---

## 3. Phase Independence Guarantee

Phase 2 is designed as a **passive consumer** of Phase 1 outputs. The following constraints are binding on all Phase 2 implementations:

**Rule 1 — Phase 1 independence.** Phase 1 must be buildable and fully operational without Phase 2 present. The Phase 1 CLI must continue to function identically regardless of whether the GUI application is installed or running.

**Rule 2 — No protocol logic in the GUI.** The GUI codebase must not contain any TLP decoding logic, protocol rule evaluation, transaction tracking, or any other form of PCIe semantic reasoning. The GUI renders what the report says; it never re-derives findings independently.

**Rule 3 — No duplication of validation rules.** The GUI must not recompute or re-infer errors from packet field values. Any error or violation displayed in the GUI must originate directly from the corresponding field in the loaded report.

**Rule 4 — Report schema as the sole interface.** Phase 2 depends exclusively on the Phase 1 report schema as defined in PCIE-SPEC-PHASE-01, Section 12. It must not depend on Phase 1 source headers, internal data structures, shared libraries, or any implementation detail beyond the schema contract.

**Rule 5 — No re-processing on user interaction.** User interactions — sorting, filtering, selecting, navigating — operate entirely on the in-memory view model constructed at load time. No interaction triggers re-reading the file, re-parsing JSON, or re-evaluating any rule.

---

## 4. Functional Requirements

### 4.1 Report Loading

| ID | Requirement |
|---|---|
| FR-GUI-LOAD-01 | The application shall allow the user to open a report file through a file picker dialog accessible from the menu, or by supplying a file path as a command-line argument at application launch. |
| FR-GUI-LOAD-02 | Upon selecting or receiving a report file path, the application shall parse the file and construct the complete in-memory view model before displaying any packet data. |
| FR-GUI-LOAD-03 | The application shall read and validate the `schema_version` field before processing any other part of the report. If the schema version is not in the set of supported versions, the application shall display a descriptive error dialog identifying the unsupported version and return to the unloaded state without displaying any partial data. |
| FR-GUI-LOAD-04 | If the report file contains malformed JSON or malformed XML that cannot be parsed, the application shall display a descriptive error dialog identifying the file path and the nature of the parse failure, then return to the unloaded state. |
| FR-GUI-LOAD-05 | For report files whose parsing and view model construction takes longer than 500 milliseconds, the application shall display a visible loading progress indicator that blocks interaction with the packet table until loading is complete. |
| FR-GUI-LOAD-06 | Before displaying any data from a newly loaded report, the application shall clear all views — the packet table, the detail panel, the error summary panel, and the statistics panel — of any data from a previously loaded report. No residual data from a prior load shall remain visible. |

### 4.2 Packet Table View

| ID | Requirement |
|---|---|
| FR-GUI-PKT-01 | The packet table shall display one row per packet contained in the loaded report, presented in ascending packet index order by default. |
| FR-GUI-PKT-02 | The table shall display the following columns for each packet: Index, Timestamp (ns), Direction, Type, Address, Length in Double Words, Tag, Status, and an Error indicator. |
| FR-GUI-PKT-03 | The Error indicator column shall show a distinct visual symbol when the packet has one or more decode errors or is referenced by one or more validation errors. |
| FR-GUI-PKT-04 | Rows corresponding to malformed packets shall be rendered with a visually distinct background color that clearly differentiates them from rows for structurally valid packets. |
| FR-GUI-PKT-05 | Rows corresponding to packets referenced by validation errors — but that are structurally valid — shall be rendered with a separate, distinguishable visual treatment that is distinct from the malformed-packet treatment. |
| FR-GUI-PKT-06 | Single-clicking a row shall select that row and immediately update the detail panel to show the full field data and error information for the selected packet. |
| FR-GUI-PKT-07 | The table shall support column-header-click sorting for every column. Clicking a column header once shall sort ascending; clicking it again shall sort descending. The current sort column and direction shall be visually indicated. |
| FR-GUI-PKT-08 | The table shall support text-based filtering by TLP type, direction, and error presence through dedicated filter controls above the table. Applying a filter shall update the visible row count indicator. |
| FR-GUI-PKT-09 | The table shall use virtual rendering to maintain smooth scrolling performance for reports containing up to one million packets. Only the rows visible in the current viewport shall be rendered in the display hierarchy at any given time. |

### 4.3 Packet Detail Panel

| ID | Requirement |
|---|---|
| FR-GUI-DET-01 | The detail panel shall display every field of the currently selected packet, including all decoded TLP header fields as enumerated in PCIE-SPEC-PHASE-01, Section 8.2. |
| FR-GUI-DET-02 | Fields whose value is `null` — because they are structurally inapplicable to the selected packet's TLP type — shall be displayed as an em-dash (`—`). The literal string `"null"` must never appear in the panel. |
| FR-GUI-DET-03 | If the selected packet has decode errors, each error shall be listed in the panel with its `rule_id`, the affected `field` name, and the full `description`. |
| FR-GUI-DET-04 | If the selected packet is referenced by one or more validation errors, each such error shall be listed in the panel with its `rule_id`, `category`, and `description`. |
| FR-GUI-DET-05 | The detail panel is entirely read-only. No field label, no field value, and no error description shall be editable. |
| FR-GUI-DET-06 | When no packet is selected — either because no report is loaded, or because the user has not yet clicked a row — the detail panel shall display an appropriate placeholder message indicating that no packet is selected. |

### 4.4 Error Summary Panel

| ID | Requirement |
|---|---|
| FR-GUI-ERR-01 | The error summary panel shall list all validation errors from the loaded report, grouped by their error category. |
| FR-GUI-ERR-02 | Each entry in the error list shall display the `rule_id`, the `category`, the `packet_index` of the primary packet, and a truncated version of the `description` field. |
| FR-GUI-ERR-03 | Double-clicking any entry in the error list shall cause the packet table to scroll to and select the row corresponding to the `packet_index` of that entry, and shall update the detail panel accordingly. |
| FR-GUI-ERR-04 | The panel shall provide a filter control allowing the user to show only errors belonging to a specific category. |
| FR-GUI-ERR-05 | The total count of validation errors in the loaded report shall be permanently visible in the panel's header area. |

### 4.5 Statistics Panel

| ID | Requirement |
|---|---|
| FR-GUI-STAT-01 | The statistics panel shall display all fields from the `summary` section of the loaded report. |
| FR-GUI-STAT-02 | The TLP type distribution shall be presented as both a numeric table showing the count per type and a proportional chart — such as a pie chart or stacked bar — showing the relative distribution visually. |
| FR-GUI-STAT-03 | The statistics panel shall be read-only and shall update only when a new report is loaded. It does not respond to packet selection or filter changes. |

---

## 5. Non-Functional Requirements

| ID | Category | Requirement |
|---|---|---|
| NFR-GUI-01 | Performance — Load Time | A report containing one hundred thousand packets shall complete parsing and view model construction and display the packet table within five seconds on a system with a warm filesystem cache. |
| NFR-GUI-02 | Performance — Scrolling | The packet table shall sustain smooth scrolling at or above 60 frames per second for any report containing up to one million packets, assuming virtual rendering is employed. |
| NFR-GUI-03 | Reliability | No combination of report content, file size, or user interaction sequence shall cause the application to crash or enter an unresponsive state. |
| NFR-GUI-04 | Reliability | All error conditions during report loading shall result in a descriptive, recoverable error dialog. The application shall always return to a stable, operable state after any load failure. |
| NFR-GUI-05 | Portability | The GUI application shall build and run on Linux x86-64 with the same kernel version constraints as Phase 1. |
| NFR-GUI-06 | Correctness | Every field value displayed in the GUI shall be derived faithfully from the loaded report. The GUI shall introduce no rounding, truncation, or reinterpretation of numeric values except for display formatting. |
| NFR-GUI-07 | Maintainability | No GUI component shall contain protocol logic. Any change to PCIe protocol rules must be achievable by changing only Phase 1 components and the report schema, with zero changes required in Phase 2. |

---

## 6. Architectural Design

### 6.1 Separation of Concerns

Phase 2 introduces two new architectural concerns that sit entirely above the Phase 1 engine: **report parsing** and **presentation**. Neither concern is permitted to overlap with or influence the Phase 1 analytical core.

```
┌──────────────────────────────────────────────────────┐
│               Phase 1 Analysis Engine                │
│   (PacketDecoder, ProtocolValidator, ReportBuilder)  │
│               Unchanged from Phase 1                 │
├──────────────────────────────────────────────────────┤
│              Report Schema Contract                  │
│        JSON / XML analysis report (v1.0)             │
├──────────────────────────────────────────────────────┤
│              GUI Report Parser                       │
│   Deserializes report → typed view model objects     │
├──────────────────────────────────────────────────────┤
│              GUI Presentation Layer                  │
│   Packet Table, Detail Panel, Statistics, Errors     │
└──────────────────────────────────────────────────────┘
```

### 6.2 View Model Pattern

The GUI does not render report objects directly. On load, the report is parsed into a set of **view model** objects — lightweight, GUI-optimized representations derived from report data. The view models are the sole data source for all panels. This design isolates the panels from changes to the report schema (only the parser and view model derivation logic need to change) and eliminates the risk of a panel accidentally modifying report data.

### 6.3 Event-Driven Interaction

All user interactions — row selection, sorting, filtering, error panel navigation — are handled through an event-driven model. Events flow from user actions to controllers, which update the relevant view model state, which in turn updates the display. No panel communicates directly with another panel; inter-panel coordination is mediated by the selection state maintained in the application controller.

### 6.4 Phase 1 Independence

Phase 2 has no compile-time or runtime dependency on Phase 1 source files. The only artifact it consumes from Phase 1 is the report file, which is a plain text document on the filesystem. Phase 1 can be built, executed, and used with no Phase 2 component present, and Phase 2 can be launched, loaded, and used with no Phase 1 process running.

---

## 7. Component Specifications

### 7.1 ReportParser

**Single Responsibility:** Read a report file from a given filesystem path, validate its structure and schema version, and produce a fully typed `ReportModel` object ready for view model derivation.

**Behavioral Contract:**
- Accepts a filesystem path at invocation time.
- Returns a `ReportModel` on success.
- Raises `UnsupportedSchemaVersionException` if `schema_version` is not in the supported set.
- Raises `ReportParseException` if the file content cannot be parsed as valid JSON or XML.
- Raises `MissingRequiredFieldException` if a mandatory report field is absent.
- Performs no protocol interpretation. Field values are preserved exactly as they appear in the report.

### 7.2 ViewModelFactory

**Single Responsibility:** Transform a `ReportModel` into the full set of view model objects required by all panels — a `PacketViewModelList`, an `ErrorViewModelList`, and a `StatisticsViewModel`.

**Behavioral Contract:**
- Accepts a `ReportModel` and produces view model collections.
- Derives display-ready strings for all fields, including em-dash substitution for null values.
- Computes per-packet error flags (`has_decode_error`, `has_validation_error`) that are used by the packet table for row coloring.
- Performs no protocol logic. Derivation is purely a mapping and formatting operation over existing report data.

### 7.3 PacketTableController

**Single Responsibility:** Manage the packet table's current view state, including sort column, sort direction, active filter set, and the currently selected row index.

**Behavioral Contract:**
- Maintains the sorted and filtered subset of the `PacketViewModelList` that is currently visible.
- Provides operations for applying a new sort specification, applying a filter predicate, and setting the selected row index.
- Emits a selection-changed event when the selected row changes, allowing the detail panel to update accordingly.
- Never performs any file I/O or report parsing.

### 7.4 DetailPanelController

**Single Responsibility:** Respond to selection-changed events from the `PacketTableController` and update the detail panel display with the fields and errors of the newly selected packet.

**Behavioral Contract:**
- Subscribes to selection-changed events.
- On each event, retrieves the `PacketViewModel` for the selected index and pushes its field data and error data to the detail panel view.
- When no packet is selected, the detail panel view is reset to the placeholder state.

### 7.5 ErrorSummaryController

**Single Responsibility:** Present the `ErrorViewModelList` in the error summary panel, support category-based filtering, and handle double-click navigation events by signaling the `PacketTableController` to scroll to and select the target packet.

### 7.6 StatisticsPanelController

**Single Responsibility:** Populate the statistics panel from the `StatisticsViewModel` derived at load time. Update only when a new report is loaded.

---

## 8. View Model Definitions

### 8.1 PacketViewModel

`PacketViewModel` is the display-optimized representation of a single packet entry.

| Field | Derived From | Display Format |
|---|---|---|
| `index` | `packet.index` | Decimal integer string |
| `timestamp_display` | `packet.timestamp_ns` | Decimal string; `—` if value is zero |
| `direction` | `packet.direction` | `TX` or `RX` |
| `tlp_type` | `packet.tlp.type` | Type name string, e.g. `MRd`, `CplD` |
| `address_display` | `packet.tlp.address` | Hex string with `0x` prefix; `—` if null |
| `length_display` | `packet.tlp.length_dw` | Decimal integer string; `—` if null |
| `tag_display` | `packet.tlp.tag` | Decimal integer string; `—` if null |
| `status_display` | `packet.tlp.status` | `SC`, `UR`, `CA`, or `—` if null |
| `has_decode_error` | `packet.is_malformed` | Boolean; drives row background color |
| `has_validation_error` | Derived from validation error index | Boolean; drives row background color |
| `has_any_error` | Logical OR of above two | Boolean; drives error indicator icon |

### 8.2 ErrorViewModel

| Field | Derived From |
|---|---|
| `rule_id` | `validation_error.rule_id` |
| `category` | `validation_error.category` |
| `packet_index` | `validation_error.packet_index` |
| `description_truncated` | `validation_error.description`, truncated to 100 characters for display |

### 8.3 StatisticsViewModel

| Field | Derived From |
|---|---|
| `total_packets` | `summary.total_packets` |
| `malformed_packet_count` | `summary.malformed_packet_count` |
| `validation_error_count` | `summary.validation_error_count` |
| `skipped_line_count` | `summary.skipped_line_count` |
| `tlp_type_distribution` | `summary.tlp_type_distribution` — a map of type name to count |

---

## 9. View Layout and Panel Specifications

### 9.1 Application Layout

```
┌──────────────────────────────────────────────────────┐
│  Menu Bar: File  View  Help                          │
├──────────────────────────────────────────────────────┤
│  [Type Filter ▾]  [Direction Filter ▾]  [Errors □]   │
├──────────────────────────────────────────────────────┤
│                                    │                  │
│        Packet Table (70%)          │   Detail Panel   │
│                                    │   (30%)          │
│                                    │                  │
├────────────────────────┬───────────┴──────────────────┤
│  Error Summary (50%)   │   Statistics Panel (50%)     │
└────────────────────────┴──────────────────────────────┘
```

### 9.2 Packet Table Column Specifications

| Column | Source Field | Default Width | Sortable | Sort Type |
|---|---|---|---|---|
| Index | `packet.index` | 80 px | Yes | Numeric |
| Timestamp (ns) | `timestamp_display` | 140 px | Yes | Numeric |
| Direction | `direction` | 70 px | Yes | Alphabetic |
| Type | `tlp_type` | 80 px | Yes | Alphabetic |
| Address | `address_display` | 140 px | Yes | Hexadecimal |
| Length (DW) | `length_display` | 80 px | Yes | Numeric |
| Tag | `tag_display` | 60 px | Yes | Numeric |
| Status | `status_display` | 70 px | Yes | Alphabetic |
| Errors | `has_any_error` | 60 px | Yes | Error rows sorted first |

### 9.3 Row Visual Convention

| Condition | Visual Treatment |
|---|---|
| Structurally valid packet, no validation errors | Default background |
| Packet referenced by a validation error | Amber or yellow tint |
| Malformed packet (decode error present) | Red tint |
| Packet is both malformed and validation-error-referenced | Red tint takes precedence |
| Currently selected row | System selection highlight color, overriding background tint |

---

## 10. User Interaction Model

### 10.1 Supported Interaction Definitions

| Interaction | Result |
|---|---|
| File → Open | Opens a file picker dialog; the selected file is parsed and loaded |
| File → Close | Clears all panels and returns the application to the unloaded state |
| Click column header | Applies sort by that column; toggles ascending and descending on successive clicks |
| Click filter control | Applies the selected filter criterion; updates the visible row count display |
| Click a packet row | Selects the row; updates the detail panel with that packet's data |
| Double-click an error entry | Scrolls the packet table to the error's target packet and selects it |
| Drag column border | Resizes the column; width is preserved for the current session |

### 10.2 Keyboard Shortcut Definitions

| Key Combination | Action |
|---|---|
| `Ctrl+O` | Open the file picker dialog |
| `Ctrl+W` | Close the currently loaded report |
| `Ctrl+Q` | Quit the application |
| `Up Arrow` | Move selection to the previous row in the packet table |
| `Down Arrow` | Move selection to the next row in the packet table |
| `Enter` | Confirm and activate the currently focused row |
| `Escape` | Clear the active filter and restore all rows |

---

## 11. Error Handling Strategy

### 11.1 Load-Time Error Categories

Every error condition encountered during report loading shall produce a descriptive modal error dialog and leave the application in the stable, unloaded state. No partial data from a failed load shall appear in any panel.

| Error Condition | Dialog Content | Application State |
|---|---|---|
| Report file not found at the specified path | File path and "file not found" explanation | Unloaded |
| Unsupported `schema_version` value | The version value read and the supported version set | Unloaded |
| JSON or XML parse failure | File path and parser error description | Unloaded |
| Missing required report field | Field name and its expected location in the schema | Unloaded |
| Report too large to display (packet count exceeds rendering capacity) | Packet count and summary-only mode offer | Optionally loads summary only |

### 11.2 Graceful Degradation Rules

- If the `packets` array is absent or empty in the report, the packet table displays a placeholder message. All other panels display their zero-state representations.
- If the `summary` section is absent from the report, the statistics panel displays `—` for all value fields.
- If any individual `PacketViewModel` field cannot be derived — because an unexpected null appears in a field that is required for the selected packet's TLP type — the affected cell displays `—` rather than propagating an error.
- No graceful degradation scenario may leave the application in an unresponsive state or in a state where further report loading is impossible.

---

## 12. Testing Strategy

### 12.1 Functional Tests

| Test | Input Data | Expected Outcome |
|---|---|---|
| Load canonical valid report | Phase 1 canonical valid report | All packets displayed in index order; statistics match report summary exactly |
| Load report with malformed packets | Report containing DEC-001 entries | Malformed rows rendered with red background tint; detail panel shows decode errors for those rows |
| Load report with validation errors | Report containing VAL-002 entries | Affected rows rendered with amber tint; error summary panel populated; statistics show correct error count |
| Navigate via error summary | Double-click error entry | Packet table scrolls to and selects the target packet; detail panel updates |
| Sort by each column | Click each column header twice | Rows reorder correctly in ascending then descending order for all nine columns |
| Filter by TLP type | Select "MRd" in the type filter | Only MRd rows remain visible; row count indicator updates |
| Detail panel null fields | Packet with null address | Address cell displays `—`, not `"null"` |

### 12.2 Robustness Tests

| Test | Input Data | Expected Outcome |
|---|---|---|
| Empty JSON document | `{}` | Error dialog displayed; application remains in unloaded state |
| Unknown schema version | `"schema_version": "99.0"` | Error dialog identifies unsupported version; unloaded state |
| Binary file selected | Non-text binary content | Parse error dialog; application remains running and operable |
| Maximum packet count | Report with 1,000,000 packets | Loads within NFR-GUI-01 time bound; scrolling meets NFR-GUI-02 |
| Empty packets array | Valid report, `"packets": []` | All panels show placeholder content; statistics show zero values |

---

## 13. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation Strategy |
|---|---|---|---|
| Virtual rendering implementation fails to sustain scroll performance for 1M+ packet reports | Medium | Medium | Prototype the table rendering component independently with synthetic view model data before integrating with real reports. Validate scroll frame rate against NFR-GUI-02 before integration is complete. |
| GUI toolkit license is incompatible with the Traceon project license | Low | High | Select and verify the toolkit license before any code is written. Consider Qt LGPL or wxWidgets as primary candidates. |
| Detail panel updates become a performance bottleneck when the user scrolls rapidly through the packet table | Low | Low | Debounce the selection-changed event with a short delay (50 milliseconds or similar) so that the detail panel update is deferred until the user pauses on a row. |
| Phase 1 report schema changes between versions break the GUI parser | Medium | High | Enforce strict `schema_version` checking at load time. Maintain a separate parser implementation for each supported schema version. When a new schema version is introduced, do not modify the existing parser — add a new one. |

---

## 14. Exit Criteria

Phase 2 is considered complete and accepted when all of the following conditions are simultaneously satisfied:

- [ ] The GUI successfully loads and correctly displays the Phase 1 canonical valid report with all packets, statistics, and zero-error state.
- [ ] Malformed packet rows are rendered with a visually distinct treatment that is unambiguously different from clean packet rows.
- [ ] Validation-error packet rows are rendered with a visually distinct treatment that is different from both clean and malformed rows.
- [ ] The detail panel correctly displays all header fields and all associated errors for every selectable packet in the canonical report.
- [ ] The error summary panel correctly navigates to the target packet on double-click for every entry in the error list.
- [ ] The statistics panel values match the `summary` section of the loaded report exactly.
- [ ] All filter and sort operations produce results consistent with the underlying view model data for all columns and all filter types.
- [ ] Loading a report with an unsupported schema version produces a descriptive error dialog and does not crash the application.
- [ ] NFR-GUI-01 (load time) and NFR-GUI-02 (scroll performance) are both met and verified against the 1M-packet test report.
- [ ] Phase 1 CLI builds, executes, and produces correct reports with zero modifications to Phase 1 source files.


