# Phase 4 — Concurrency and Live Data Processing

**Document ID:** PCIE-SPEC-PHASE-04  
**Project:** Traceon   
**Version:** 2.0.0  
**Status:** Approved  
**Phase Dependencies:** Phase 1 (PCIE-SPEC-PHASE-01) — the `PacketDecoder` and `ProtocolValidator` components are consumed unchanged. Phase 2 (PCIE-SPEC-PHASE-02) is optional; the GUI live-update model defined here applies when Phase 2 is present.  
**Consumed By:** Phase 5 (live transaction modeling), Phase 6 (live fault injection integration), Phase 7 (multi-protocol live streams).  

---

## Table of Contents

1. [Purpose and Scope](#1-purpose-and-scope)
2. [Glossary](#2-glossary)
3. [Phase Independence Guarantee](#3-phase-independence-guarantee)
4. [Functional Requirements](#4-functional-requirements)
5. [Non-Functional Requirements](#5-non-functional-requirements)
6. [Architectural Design](#6-architectural-design)
7. [Asio Integration Design](#7-asio-integration-design)
8. [Component Specifications](#8-component-specifications)
9. [QueueManager Specification](#9-queuemanager-specification)
10. [Thread Model](#10-thread-model)
11. [Live Data Framing Protocol](#11-live-data-framing-protocol)
12. [GUI Live-Update Model](#12-gui-live-update-model)
13. [Configuration Reference](#13-configuration-reference)
14. [Error Handling Strategy](#14-error-handling-strategy)
15. [Testing Strategy](#15-testing-strategy)
16. [Risks and Mitigations](#16-risks-and-mitigations)
17. [Exit Criteria](#17-exit-criteria)

---

## 1. Purpose and Scope

### 1.1 Purpose

Phase 4 extends the Traceon platform to support **concurrent processing** and **live data ingestion**. It introduces Asio-based asynchronous networking that enables the analyzer to receive raw PCIe packets in real time from hardware capture taps, simulation environments, or forwarding proxies over a TCP connection. Critically, the Phase 1 analysis core — the `PacketDecoder` and `ProtocolValidator` — is consumed entirely unchanged. Phase 4 builds the infrastructure that feeds the existing engine, not a replacement for it.

### 1.2 Objectives

- Introduce a thread-safe `QueueManager` as the single, controlled synchronization point between producers (network sessions and file readers) and the Phase 1 analysis consumer.
- Integrate Asio (`asio::io_context`, `tcp::acceptor`, `async_read`) for non-blocking, asynchronous network packet reception on a dedicated ingestion thread.
- Ensure that single-threaded offline mode — the complete Phase 1 behavior — remains the default mode and is entirely unaffected by the presence of the Phase 4 components.
- Provide a thread-safe event channel for incremental GUI updates when Phase 2 is present, allowing the packet table to grow in real time without blocking the analysis thread.
- Guarantee output determinism: identical input packets arriving through either offline file reading or live TCP delivery must produce identical analysis reports.

### 1.3 In-Scope Capabilities

| Capability | Description |
|---|---|
| Concurrent producer–consumer pipeline | Packet ingestion and analysis run on separate threads bridged by the QueueManager |
| Asio TCP live ingestion | Asynchronous receipt of raw TLP packets over a TCP connection |
| Thread-safe QueueManager | A bounded MPSC queue that decouples producers from the Phase 1 consumer |
| Offline mode preservation | Single-threaded path through Phase 1 unchanged; no queue or threads involved |
| Asynchronous GUI updates | Decoded TLPs emitted to a separate event ring buffer for the GUI thread |
| Configurable concurrency | All threading is opt-in; the default mode remains single-threaded offline |

### 1.4 Explicitly Out of Scope

| Excluded Capability | Rationale |
|---|---|
| UDP ingestion | Framing without connection guarantees requires a separate design; deferred to a future phase |
| Parallel validation across multiple validator threads | The `ProtocolValidator` is stateful and single-threaded by design |
| Hardware PCIe tap device drivers | Platform-specific kernel-level concerns outside software analyzer scope |
| TLM modeling during live ingestion | Phase 5 responsibility |
| Fault injection during live ingestion | Phase 6 responsibility |

---

## 2. Glossary

| Term | Definition |
|---|---|
| **Asio** | An asynchronous I/O library (either Boost.Asio or standalone Asio) providing `io_context`, async TCP operations, and strand-based handler serialization. |
| **`io_context`** | Asio's event loop. Receives completion notifications from the operating system and dispatches registered handlers. Runs exclusively on the Ingestion Thread. |
| **`tcp::acceptor`** | An Asio object that binds to a configured host/port and listens for incoming TCP connections. |
| **`async_read`** | An Asio operation that registers a non-blocking read for a specified number of bytes and invokes a completion handler when the data is available. |
| **Strand** | An Asio execution serializer that ensures completion handlers associated with the same strand are never executed concurrently, eliminating the need for explicit mutexes within a single `io_context`. |
| **AsioSession** | A component managing the lifecycle of one TCP connection — frame reading, packet reconstruction, and queue submission — for a single connected sender. |
| **QueueManager** | A bounded, thread-safe queue that decouples the Ingestion Thread (producer side) from the Consumer Thread (consumer side). |
| **MPSC** | Multiple-Producer Single-Consumer. The queue topology employed in Phase 4: multiple concurrent ingestion sources may push packets, but only the single Consumer Thread may pop them. |
| **Backpressure** | The mechanism by which a saturated queue throttles upstream producers rather than discarding packets. When the queue is full, producers block until space is available. |
| **Length-prefix framing** | The wire protocol used on the TCP connection. Each packet transmission is preceded by a fixed-size integer field specifying the byte length of the packet payload that follows. |
| **GuiEventChannel** | A lock-free single-producer single-consumer ring buffer used to push decoded TLPs from the Consumer Thread to the GUI Thread without blocking either. |
| **Offline mode** | The default Phase 1 processing mode. The `TraceFileReader` invokes the decoder and validator directly in a single thread; no queue, no thread, no network socket. |
| **Live mode** | The Phase 4 concurrent processing mode, activated by the `--live` CLI argument. The `AsioSession` and optionally the `TraceFileReader` push packets to the `QueueManager`; the `ConsumerLoop` pops and processes them. |
| **ConsumerLoop** | The single thread that pops `RawPacket` objects from the `QueueManager` and drives the Phase 1 decode-validate-report pipeline. |

---

## 3. Phase Independence Guarantee

The following constraints are binding on Phase 4 and all subsequent phases:

**Rule 1 — Phase 1 components are not modified.** The `PacketDecoder` and `ProtocolValidator` are consumed exactly as delivered in Phase 1. Their interfaces, behaviors, and internal designs are unchanged. The Phase 4 infrastructure adapts around them.

**Rule 2 — Offline mode produces identical results to Phase 1 alone.** The introduction of queues, threads, and networking infrastructure must have zero effect on the analysis outputs for file-based input. Offline mode reports must be byte-identical to those produced by Phase 1 in isolation.

**Rule 3 — Live mode is opt-in.** The system defaults to offline (single-threaded) mode. Live mode is activated exclusively through explicit configuration at startup. No live mode component is instantiated in offline mode.

**Rule 4 — The QueueManager is the sole shared state.** All synchronization between the Ingestion Thread and the Consumer Thread is encapsulated within the `QueueManager`. No other mutable state may be accessed from more than one thread without going through a synchronized interface.

**Rule 5 — The GUI event channel is write-only from the Consumer Thread.** The Consumer Thread never waits on the GUI Thread, and the GUI Thread never writes to the channel. The channel is strictly unidirectional to prevent any priority inversion or deadlock between the analysis path and the display path.

---

## 4. Functional Requirements

### 4.1 Execution Mode Selection

| ID | Requirement |
|---|---|
| FR-CONC-01 | The system shall support two mutually exclusive execution modes: `offline` (single-threaded, full Phase 1 behavior) and `live` (multi-threaded with Asio TCP ingestion). |
| FR-CONC-02 | The execution mode shall be determined at startup by the presence or absence of the `--live` CLI argument. In the absence of `--live`, the system shall always default to offline mode. |
| FR-CONC-03 | In offline mode, the system shall not spawn any threads, open any network sockets, or instantiate any Phase 4-specific components. The processing path shall be indistinguishable from a standalone Phase 1 execution. |
| FR-CONC-04 | In live mode, the system shall start the Ingestion Thread and the Consumer Thread before opening the TCP acceptor and before accepting any incoming connections. |

### 4.2 Asio TCP Ingestion

| ID | Requirement |
|---|---|
| FR-ASIO-01 | In live mode, the system shall open a TCP listening socket on the host address and port number specified in the live configuration. |
| FR-ASIO-02 | The system shall use length-prefix framing as defined in Section 11 to reconstruct complete `RawPacket` objects from the TCP byte stream. |
| FR-ASIO-03 | All socket read operations shall be performed using `async_read` with Asio completion handlers. Synchronous or blocking socket reads are strictly prohibited on the Ingestion Thread. |
| FR-ASIO-04 | Upon reconstructing a complete `RawPacket` from the framed stream, the `AsioSession` shall push the packet to the `QueueManager`. |
| FR-ASIO-05 | If the `QueueManager` is at capacity when a push is attempted, the `AsioSession` shall not post the next `async_read` operation until the push succeeds, thereby applying backpressure to the upstream sender. |
| FR-ASIO-06 | Phase 4 shall support at minimum one simultaneous TCP sender connection. The design shall not preclude support for multiple connections in future phases. |
| FR-ASIO-07 | If the TCP sender closes the connection cleanly, the `AsioSession` shall signal the `QueueManager` that no further packets will arrive from that connection, allowing the Consumer Loop to finalize processing after draining the remaining queue contents. |

### 4.3 QueueManager Behavior

| ID | Requirement |
|---|---|
| FR-QUEUE-01 | The `QueueManager` shall provide a thread-safe `push(RawPacket)` operation callable from any producer thread. |
| FR-QUEUE-02 | The `QueueManager` shall provide a `pop()` operation that blocks the calling thread until a packet is available or a shutdown signal has been received. |
| FR-QUEUE-03 | The `QueueManager` shall be bounded. The maximum number of packets it may hold simultaneously shall be configurable, with a default capacity of 1,024 packets. |
| FR-QUEUE-04 | `push()` shall block when the queue is at its capacity limit, providing implicit flow control to all producers. |
| FR-QUEUE-05 | The `QueueManager` shall support a `shutdown()` operation that causes all threads currently blocked in `pop()` to return immediately with a sentinel value indicating that shutdown is in progress. |
| FR-QUEUE-06 | After `shutdown()` has been called, `push()` shall return immediately without enqueuing the packet, preventing producers from blocking indefinitely during shutdown. |

### 4.4 Consumer Loop Behavior

| ID | Requirement |
|---|---|
| FR-CONS-01 | The Consumer Loop shall run on a dedicated thread and continuously pop `RawPacket` objects from the `QueueManager`. |
| FR-CONS-02 | For each popped packet, the Consumer Loop shall invoke `PacketDecoder.decode()`, pass the result to `ReportBuilder.add_tlp()`, and — if the TLP is not malformed — pass it to `ProtocolValidator.process()`. |
| FR-CONS-03 | When the `QueueManager.pop()` returns the shutdown sentinel, the Consumer Loop shall call `ProtocolValidator.finalize()`, collect all validation errors, pass them to the `ReportBuilder`, and then invoke `ReportBuilder.write()` to produce the final report. |
| FR-CONS-04 | If Phase 2 is present and the `GuiEventChannel` is configured, the Consumer Loop shall attempt to push each decoded TLP to the channel after processing it. If the channel is full, the push shall be skipped silently. |

---

## 5. Non-Functional Requirements

| ID | Category | Requirement |
|---|---|---|
| NFR-CONC-01 | Correctness | Offline mode must produce byte-identical analysis reports to Phase 1 standalone for all canonical trace files, without exception. |
| NFR-CONC-02 | Thread Safety | The `QueueManager` implementation must pass all tests under ThreadSanitizer (TSan) with zero detected data races. Any TSan error is treated as a blocking defect. |
| NFR-CONC-03 | Reliability | An exception thrown in any background thread must be propagated to the main thread and handled gracefully. Background thread crashes must not silently discard analysis results or leave the process in an indeterminate state. |
| NFR-CONC-04 | Reliability | The system must complete cleanly — flushing all queued packets, calling `finalize()`, and writing the report — in all shutdown scenarios: clean sender disconnect, timeout, and user-initiated termination. |
| NFR-CONC-05 | Determinism | Analysis results must not depend on thread scheduling. The same sequence of packets delivered in the same order must always produce the same report, regardless of thread interleaving timing. |
| NFR-CONC-06 | Performance | The live ingestion pipeline must sustain a processing rate of at least 100,000 packets per second on the reference system without dropping packets under backpressure. |

---

## 6. Architectural Design

### 6.1 System Thread Map

Phase 4 introduces a fixed three-thread model for live mode operation.

```
Main Thread
│  Parses CLI arguments, instantiates components,
│  starts Ingestion and Consumer threads,
│  waits for shutdown signal,
│  checks thread exception futures on exit.
│
├─── Ingestion Thread
│      Runs asio::io_context event loop.
│      AsioSession performs async_read, reconstructs frames,
│      pushes RawPackets to QueueManager.
│
└─── Consumer Thread
       ConsumerLoop pops from QueueManager.
       Drives PacketDecoder → ProtocolValidator → ReportBuilder.
       Optionally writes to GuiEventChannel.
       Calls finalize() and write() on shutdown sentinel.
```

### 6.2 Data Flow in Live Mode

```
TCP Sender → AsioSession (async_read) → QueueManager.push()
                                              │
                                        QueueManager.pop()
                                              │
                                       ConsumerLoop
                                              │
                          ┌─────────────────────────────────────┐
                          │  PacketDecoder.decode(RawPacket)     │
                          │          → TLP                       │
                          ├─────────────────────────────────────┤
                          │  ReportBuilder.add_tlp(TLP)          │
                          ├─────────────────────────────────────┤
                          │  ProtocolValidator.process(TLP)      │
                          │  (if not malformed)                  │
                          ├─────────────────────────────────────┤
                          │  GuiEventChannel.try_push(TLP)       │
                          │  (if Phase 2 present, non-blocking)  │
                          └─────────────────────────────────────┘
```

### 6.3 Offline Mode Data Flow

In offline mode, the entire Phase 4 infrastructure is absent. The processing path follows exactly the Phase 1 orchestration loop: `TraceFileReader` → `PacketDecoder` → `ProtocolValidator` → `ReportBuilder` on the main thread with no queuing and no threading.

---

## 7. Asio Integration Design

### 7.1 io_context Ownership

One `asio::io_context` instance is created by the `LiveModeOrchestrator` and passed to all Asio-aware components. The `io_context` runs exclusively on the Ingestion Thread. No Asio operations are initiated from the Consumer Thread or the Main Thread after startup.

### 7.2 Connection Lifecycle

The `tcp::acceptor` listens on the configured host/port. When a connection arrives, it creates an `AsioSession` to manage that connection's packet stream. The `AsioSession` initiates the first `async_read` for the length-prefix field. On completion, it reads the payload, constructs a `RawPacket`, and pushes it to the `QueueManager`. It then initiates the next length-prefix read. This chain continues until the connection is closed or an error occurs.

### 7.3 Session Memory Management

`AsioSession` instances are managed through `shared_ptr`. The `tcp::acceptor` holds a `shared_ptr` to each active session. When a session closes, the `shared_ptr` reference count reaches zero and the session is destroyed. Back-references from session objects to the acceptor or the `io_context` use `weak_ptr` to prevent reference cycles that would prevent cleanup.

### 7.4 Strand Usage

All completion handlers associated with a given `AsioSession` are posted through a strand tied to that session. This ensures that even if the `io_context` were to run on multiple threads in a future extension, the per-session packet reconstruction state would never be accessed concurrently.

---

## 8. Component Specifications

### 8.1 LiveModeOrchestrator

**Single Responsibility:** Own and coordinate the lifecycle of all Phase 4 components in live mode. Create the `io_context`, `QueueManager`, `ConsumerLoop`, and `AsioSession`. Start and stop threads in the correct order. Propagate thread exceptions to the main thread.

**Behavioral Contract:**
- Constructed from a `LiveConfig` struct.
- Provides a `start()` operation that launches the Ingestion Thread and the Consumer Thread, then opens the TCP acceptor.
- Provides a `wait_for_completion()` operation that blocks the main thread until the Consumer Thread has written the report.
- Checks thread exception futures during shutdown and re-throws any captured background thread exceptions on the main thread.

### 8.2 AsioSession

**Single Responsibility:** Manage all I/O for one TCP connection. Read the length-prefix framing header, read the packet payload, construct a `RawPacket`, and push it to the `QueueManager`. Signal shutdown on clean disconnect.

**Behavioral Contract:**
- All reads use `async_read`; the session never blocks the Ingestion Thread.
- On clean disconnect, calls `QueueManager.notify_producer_done()`.
- On a frame whose declared length exceeds the maximum valid PCIe TLP size (65,535 bytes), discards the frame, logs a warning, and continues reading.

### 8.3 ConsumerLoop

**Single Responsibility:** Continuously pop `RawPacket` objects from the `QueueManager` and drive the Phase 1 analysis pipeline for each one. Perform finalization and report writing when the shutdown sentinel is received.

**Behavioral Contract:**
- Runs on the Consumer Thread.
- Invokes `PacketDecoder`, `ProtocolValidator`, and `ReportBuilder` on every packet, following the same sequencing as the Phase 1 `AnalyzerEngine`.
- Writes the report before the thread exits, ensuring the report is available when the main thread joins.

---

## 9. QueueManager Specification

### 9.1 Design

The `QueueManager` is a bounded, MPSC queue built on a `std::deque` protected by a `std::mutex` and coordinated through a pair of `std::condition_variable` instances — one for producers waiting on space, one for the consumer waiting on data. No lock-free or hand-rolled synchronization primitives are used in the core implementation; correctness is prioritized over micro-optimization.

### 9.2 Capacity and Blocking Semantics

The queue capacity is set at construction time and is immutable thereafter. When the internal packet count equals the capacity:
- Calls to `push()` block on the producer condition variable until the consumer pops at least one packet or `shutdown()` is called.
- Calls to `pop()` block on the consumer condition variable until at least one packet is available or `shutdown()` is called.

### 9.3 Shutdown Semantics

`shutdown()` sets a stop flag under the mutex, then notifies all waiting threads on both condition variables. Any thread blocked in `push()` wakes up and returns `false` immediately. Any thread blocked in `pop()` wakes up and returns a sentinel indicating shutdown. After `shutdown()` is called, subsequent `push()` calls return `false` immediately without acquiring the lock or attempting to enqueue.

### 9.4 Thread Safety Contract

Every public method — `push()`, `pop()`, `shutdown()` — is fully thread-safe and may be called concurrently from any combination of threads without additional external synchronization.

---

## 10. Thread Model

### 10.1 Thread Responsibilities Summary

| Thread | Name | Responsibilities |
|---|---|---|
| Main Thread | `main` | CLI parsing, component construction, thread lifecycle management, exception collection |
| Ingestion Thread | `asio_io` | Running `io_context.run()`, all `async_read` completions, `QueueManager.push()` |
| Consumer Thread | `consumer` | `QueueManager.pop()`, `PacketDecoder`, `ProtocolValidator`, `ReportBuilder`, `GuiEventChannel.try_push()` |

### 10.2 Shared Mutable State

The only mutable state shared across threads is the `QueueManager` itself. All other components are owned exclusively by one thread:

- `io_context`, `tcp::acceptor`, `AsioSession` objects: owned by the Ingestion Thread exclusively.
- `PacketDecoder`, `ProtocolValidator`, `ReportBuilder`: owned by the Consumer Thread exclusively.
- `GuiEventChannel`: written by Consumer Thread; read by GUI Thread (if present). No other thread may write to or read from it.

### 10.3 Exception Propagation

Every thread function wraps its entire body in a try/catch block. If an exception escapes the thread body, it is captured through a `std::promise`/`std::future` mechanism. The main thread checks the corresponding futures during shutdown and re-throws any captured exceptions, ensuring that background failures surface as observable errors rather than silent process termination.

---

## 11. Live Data Framing Protocol

### 11.1 Frame Structure

Each packet transmitted over the TCP connection is preceded by a 4-byte unsigned integer in network byte order (big-endian), specifying the number of bytes in the packet payload that immediately follows.

```
┌────────────────────┬───────────────────────────────────┐
│  Length (4 bytes)  │  Packet Payload (Length bytes)    │
│  Big-endian uint32 │  Raw bytes of the RawPacket       │
└────────────────────┴───────────────────────────────────┘
```

### 11.2 Packet Payload Encoding

The packet payload is a serialized `RawPacket` encoded as a UTF-8 JSON object with the fields `timestamp_ns`, `direction`, and `payload_hex`. This encoding matches the field structure defined in PCIE-SPEC-PHASE-01, Section 8.1, and allows the Phase 1 `TraceInputLayer` parsing logic to be reused on the deserialization path.

### 11.3 Error Conditions

| Condition | Behavior |
|---|---|
| Length field declares more than 65,535 bytes | Frame is discarded; a warning is logged identifying the declared length; reading continues at the next frame boundary |
| TCP connection closes mid-payload | The incomplete packet is discarded; a warning is logged with the byte count received; `notify_producer_done()` is called |
| Payload cannot be deserialized as a valid `RawPacket` | The packet is discarded; a warning is logged; processing continues |

---

## 12. GUI Live-Update Model

### 12.1 GuiEventChannel Design

The `GuiEventChannel` is a bounded, lock-free single-producer single-consumer ring buffer. It is write-only from the Consumer Thread and read-only from the GUI Thread. Neither thread ever blocks on the other.

- The Consumer Thread calls `try_push(TLP)` after processing each decoded TLP. If the channel is full — because the GUI is rendering more slowly than the analysis rate — the push is silently discarded. Analysis correctness is never affected.
- The GUI Thread polls the channel on a periodic timer (recommended interval: 16 milliseconds, approximately 60 Hz). Each poll call drains all currently available entries and applies them to the packet table view model.

### 12.2 GUI Behavior During Live Ingestion

When Phase 2 is present and live mode is active, the GUI operates in an eventually-consistent display mode:

- Each TLP received from the `GuiEventChannel` is appended as a new row to the packet table view model.
- Summary statistics in the statistics panel are updated incrementally after each received TLP.
- The error summary panel is updated whenever a `ValidationError` is emitted by the Consumer Thread.
- The detail panel is not refreshed automatically. It reflects the state at the time the user most recently selected a row.
- The packet table is not automatically re-sorted or re-filtered during live ingestion. Sorting and filtering operate on a snapshot of the view model taken at the moment the user explicitly requests the operation.

---

## 13. Configuration Reference

### 13.1 LiveConfig Fields

| Field | Default | Description |
|---|---|---|
| `listen_host` | `"127.0.0.1"` | IP address on which the TCP acceptor listens |
| `listen_port` | `9876` | TCP port number on which the TCP acceptor listens |
| `queue_capacity` | `1024` | Maximum number of packets the QueueManager may hold simultaneously |
| `gui_channel_capacity` | `4096` | Maximum number of TLPs the GuiEventChannel ring buffer may hold |
| `connection_timeout_s` | `30` | Seconds to wait for an initial TCP connection before reporting a warning |

### 13.2 Phase 4 CLI Arguments

| Argument | Default | Description |
|---|---|---|
| `--live` | Off | Enables live mode. When present, `--input` is ignored. |
| `--host <address>` | `127.0.0.1` | Overrides `listen_host`. |
| `--port <number>` | `9876` | Overrides `listen_port`. |
| `--queue-capacity <n>` | `1024` | Overrides `queue_capacity`. |

---

## 14. Error Handling Strategy

### 14.1 Network Error Handling

| Error Condition | Response |
|---|---|
| TCP sender not yet ready at startup | Log a warning; the acceptor remains open and will accept the connection when the sender connects |
| TCP connection closed cleanly by sender | Complete processing of all queued packets; call `finalize()`; write report normally |
| TCP connection closed abnormally mid-packet | Discard the incomplete packet; log a warning; call `notify_producer_done()`; proceed to finalization |
| Frame length exceeds maximum valid size | Discard the frame; log a warning with the declared length; continue reading |
| `io_context` throws an unhandled exception | Capture through `std::promise`; propagate to main thread via future; trigger graceful shutdown |

### 14.2 Shutdown Deadlock Prevention

If the Main Thread calls `QueueManager.shutdown()` while a producer thread is blocked in `push()`, the blocked `push()` must wake up and return `false` rather than waiting indefinitely. The `shutdown()` implementation achieves this by setting the stop flag and notifying the producer condition variable under the same mutex lock, ensuring that all blocked producers observe the stop flag on their next wait cycle.

---

## 15. Testing Strategy

### 15.1 Unit Tests

| Component | Required Test Coverage |
|---|---|
| `QueueManager` | Single producer / single consumer with exact capacity boundary; `push()` blocks at capacity and unblocks when consumer pops; `pop()` blocks at empty and unblocks when producer pushes; `shutdown()` unblocks all waiters; `push()` after shutdown returns `false` |
| `AsioSession` | Frame reassembly from split TCP segments; oversized frame rejected and next frame read correctly; clean disconnect triggers `notify_producer_done()` |
| `ConsumerLoop` | Correct invocation order (decode → validate → report) for each packet; shutdown sentinel triggers `finalize()` and `write()`; GUI channel write attempted for each non-malformed TLP |
| `GuiEventChannel` | `try_push()` returns `false` when ring buffer is full; `try_pop()` returns empty when ring buffer is empty; SPSC ordering invariant holds under sequential push/pop |

### 15.2 Integration Tests

| Test | Description | Expected Outcome |
|---|---|---|
| Offline equivalence | Process the same trace in both offline and live modes (live mode via a loopback TCP sender) | Reports are byte-identical |
| Live ingestion end-to-end | Launch analyzer in live mode; send a known trace via a test TCP sender; collect and verify report | Report matches expected output for the trace |
| Backpressure | Introduce an artificial delay in the Consumer Thread; send packets at maximum rate | Zero packets dropped; all packets processed in order; no deadlock |
| Sender disconnect | Sender closes connection after N packets; verify that analysis finalizes and report is written | Report contains all N packets; `finalize()` called once |
| High-throughput stress | Send 100,000 packets at the maximum possible rate | All packets received and processed; NFR-CONC-06 satisfied |

### 15.3 Concurrency Correctness Tests

All unit tests and integration tests for Phase 4 components must be executed under ThreadSanitizer (TSan) with zero reported data races. Any TSan error, regardless of apparent severity, is treated as a blocking defect and must be resolved before the change is accepted into the codebase.

---

## 16. Risks and Mitigations

| Risk | Likelihood | Impact | Mitigation Strategy |
|---|---|---|---|
| Race condition in `QueueManager` under stress conditions causes data corruption or deadlock | Medium | High | Mandate TSan clean pass on every change. Use only `std::condition_variable` with `std::unique_lock` for synchronization. Prohibit hand-rolled lock-free constructs in the queue implementation. |
| Consumer Thread exception silently discards the analysis report | Medium | High | Enforce the `std::promise`/`std::future` pattern for all thread bodies. Main thread checks exception state before exiting and re-throws any captured exception. |
| Asio `AsioSession` shared_ptr cycle prevents session cleanup on disconnect | Low | Medium | Use `weak_ptr` for all back-references from session objects. Audit ownership graph explicitly during code review. |
| Live mode and offline mode analysis outputs diverge for the same packet sequence | Low | High | Offline-equivalence integration test runs on every CI build. Any divergence is a blocking defect. |
| GuiEventChannel overflow during high-burst live ingestion silently degrades display accuracy | Medium | Low | Drop policy is explicit and logged. The GUI is by design eventually consistent; no analysis correctness is affected. |
| Framing protocol implementation in third-party senders is incompatible with the Phase 4 receiver | Medium | High | Publish the framing specification as a standalone interoperability document. Provide a reference test sender implementation in the Traceon repository. |

---

## 17. Exit Criteria

Phase 4 is considered complete and accepted when all of the following conditions are simultaneously satisfied:

- [ ] Offline mode produces byte-identical analysis reports to Phase 1 standalone for all canonical trace files.
- [ ] Live mode end-to-end test passes: test sender connects via TCP, sends a known trace, and the analyzer produces the correct report.
- [ ] All `QueueManager` unit tests pass under ThreadSanitizer with zero data races.
- [ ] Thread lifecycle tests pass: clean startup, clean shutdown, sender-disconnect-triggered shutdown, and exception propagation from background threads.
- [ ] Backpressure test confirms zero packet loss under a deliberately slowed consumer.
- [ ] High-throughput stress test meets NFR-CONC-06 (100,000 packets per second).
- [ ] Phase 1, Phase 2, and Phase 3 components remain entirely unmodified and fully operational.
- [ ] CLI arguments `--live`, `--host`, `--port`, and `--queue-capacity` function exactly as specified in Section 13.2.
- [ ] Live mode operation is documented in the user guide with a setup walkthrough for the TCP sender connection.

