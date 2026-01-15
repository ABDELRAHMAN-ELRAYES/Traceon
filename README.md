# Traceon — PCIe / CXL Protocol Analyzer & Verification Tool

## **Project Overview**

**Traceon** is a modular software platform for analyzing, validating, and visualizing PCIe and CXL interconnect traffic. It provides transaction-level analysis, system-level visualization, and supports automated testing and error injection through Python/Tcl scripts.

Traceon is built in **C++** for the backend, **Qt** for the GUI, and uses **Python/Tcl** for automation. It is designed to be developed **incrementally**, starting from core functionality and extending through advanced phases.

---

## **Goals**

- Parse and decode raw PCIe/CXL trace files or live streams
- Detect protocol violations (CRC errors, ordering violations, timing issues)
- Provide real-time and post-analysis visualization
- Support automated regression testing and error injection
- Enable system-level modeling using TLM/SystemC
- Support multiple protocols (PCIe Gen3+, CXL, NVMe) via a plugin architecture

---

## **Project Phases & Module Positioning**

| Phase                                                                       | Modules Introduced                                                 | Deliverables                                                                       |
| --------------------------------------------------------------------------- | ------------------------------------------------------------------ | ---------------------------------------------------------------------------------- |
| **Phase 1 — Core PCIe Protocol Analyzer**                                   | `Packet`, `TLP`, `CXLPacket`, `Validator`, basic logging           | MVP: Parse, validate, display, and log PCIe/CXL traffic                            |
| **Phase 2 — GUI Visualization Layer**                                       | `MainFrame`, `PacketTablePanel`, `ProtocolTreePanel`, `StatsPanel` | Full GUI for packet display, filtering, protocol hierarchy, and statistics         |
| **Phase 3 — Automation & Scripting Layer**                                  | `PythonAPI.cpp`, `TclAPI.cpp`                                      | Automated testing, regression scripts, report generation                           |
| **Phase 4 — Concurrency & Live Data Processing**                            | `QueueManager`, multithreading infrastructure                      | Thread-safe live trace processing with asynchronous GUI updates                    |
| **Phase 5 — Transaction-Level Modeling (TLM) & Virtual System Integration** | `TLMIntegration.h/cpp`                                             | TLM/SystemC models for simulated traffic alongside real traces                     |
| **Phase 6 — Error Injection & Fault Modeling**                              | Python/Tcl test harness integration with backend validation        | Stress testing, invalid packet injection, root-cause analysis                      |
| **Phase 7 — Multi-Protocol & Cross-Domain Modeling**                        | `PacketDecoder` modular backend                                    | Plugin-based support for PCIe Gen4/5, CXL, NVMe, and future interconnect protocols |

---

## **Technical Architecture**

**Layers and Responsibilities:**

| Layer                      | Modules                                                                    | Responsibilities                                                                |
| -------------------------- | -------------------------------------------------------------------------- | ------------------------------------------------------------------------------- |
| **Backend**                | `Packet`, `TLP`, `CXLPacket`, `Validator`, `QueueManager`, `PacketDecoder` | Parse and validate traffic, manage thread-safe queues, modular protocol support |
| **GUI**                    | `MainFrame`, `PacketTablePanel`, `ProtocolTreePanel`, `StatsPanel`         | Display packets, filter and sort, protocol tree, statistics, async updates      |
| **Automation / Scripting** | `PythonAPI.cpp`, `TclAPI.cpp`                                              | Automated testing, error injection, report generation                           |
| **Simulation / TLM**       | `TLMIntegration.h/cpp`                                                     | Transaction-level modeling for early-stage verification                         |
| **Infrastructure**         | Multithreading, networking (optional), logging                             | Concurrency, live streaming, trace storage                                      |

---

## **Project File Structure**

```
Traceon/
├─ src/
│  ├─ backend/
│  │  ├─ core/
│  │  │  ├─ Packet.h / Packet.cpp
│  │  │  ├─ TLP.h / TLP.cpp
│  │  │  └─ CXLPacket.h / CXLPacket.cpp
│  │  ├─ validation/
│  │  │  ├─ Validator.h / Validator.cpp
│  │  │  └─ QueueManager.h / QueueManager.cpp
│  │  └─ decoder/
│  │     └─ packet_decoder.h / packet_decoder.cpp
│  ├─ gui/
│  │  ├─ MainFrame.h / MainFrame.cpp
│  │  ├─ Panels/
│  │  │  ├─ PacketTablePanel.h / PacketTablePanel.cpp
│  │  │  ├─ ProtocolTreePanel.h / ProtocolTreePanel.cpp
│  │  │  └─ StatsPanel.h / StatsPanel.cpp
│  ├─ scripting/
│  │  ├─ PythonAPI.cpp
│  │  └─ TclAPI.cpp
│  ├─ tlm/
│  │  ├─ TLMIntegration.h / TLMIntegration.cpp
│  └─ main.cpp
├─ tests/
│  ├─ unit/
│  │  ├─ backend/
│  │  └─ gui/
│  └─ system/
├─ scripts/
│  └─ sample_tests.py
├─ documents/
│  └─ design_spec.md
├─ output/
│  ├─ logs/
│  └─ reports/
├─ CMakeLists.txt
└─ README.md
```

---

## **Getting Started (MVP Version)**

1. **Clone Repository**

```bash
git clone https://github.com/ABDELRAHMAN-ELRAYES/Traceon.git
cd Traceon
```

2. **Install Dependencies**

- GCC / Clang (C++17 support)
- CMake
- Qt
- Python 3.x / Tcl

3. **Build Project**

```bash
mkdir build && cd build
cmake ../
make
```

4. **Run Traceon**

```bash
./Traceon
```

5. **Load Trace Files**

- Import sample PCIe/CXL trace files
- Inspect packets in the GUI
- Verify protocol compliance and errors

---

## **Development Roadmap**

1. **Phase 1:** Core analyzer backend, parsing, validation, logging
2. **Phase 2:** GUI visualization for packet inspection and statistics
3. **Phase 3:** Automation & scripting layer for testing and report generation
4. **Phase 4:** Multithreading and live trace processing
5. **Phase 5:** TLM/SystemC integration for simulated traffic
6. **Phase 6:** Error injection and stress testing
7. **Phase 7:** Multi-protocol, plugin-based support for PCIe Gen4/5, CXL, NVMe

Each phase is **independent** and can be developed and tested before moving to the next phase.

The **MVP** can be created using **Phase 1–2**, after which the remaining phases can be incrementally added.

---

## **Contribution Guidelines**

- Follow C++ OOP principles and modular design
- Document classes and modules clearly
- Implement unit tests for backend and GUI modules
- Keep GUI isolated from backend logic
- Use Python/Tcl scripts for automated regression testing
