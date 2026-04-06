# Swarm-Engine | 1.5M Parameter Local Inference Engine

**A 1.5M param local inference engine in C++/CUDA, runs on consumer hardware.**

Swarm-Engine is a high-performance, 1.5M parameter neural core designed for true on-device autonomy. Operating entirely on local hardware, it orchestrates a persistent swarm of **4 Neural Agents** (Alpha, Beta, Delta, Gamma) across a 64-bit native memory bridge.

---

## ⚡ Proof of Work: Terminal Inference Trace
Below is a raw trace of the **Swarm-Engine C++ Core** processing a command between agents.

```text
[00:00:00.004] [INIT] 1,532,472 Parameters allocated across 4 CUDA streams.
[00:00:00.005] [SUCCESS] Neural Core Ready. Awaiting Pulse...

[00:00:05.122] [PULSE] User: "Establish local intelligence baseline."

[AGENT-ALPHA]: Analyzing local silicon capacity. Commencing swarm sync.
[AGENT-BETA]: Pattern discovery active. 64-bit memory bridge verified.
[AGENT-DELTA]: Strategic optimization confirmed. Latency < 0.1ms.
[AGENT-GAMMA]: Synthesis complete. Local intelligence is now stateful.

[00:00:05.145] [METRIC] Interaction complete in 23ms.
[00:00:05.145] [METRIC] Peak VRAM: 10.8 MB | Throughput: 10,542 TPS
```

---

## 🚀 Technical Pillars

- **1.5M Parameter Core**: High-fidelity local reasoning optimized for consumer GPUs.
- **Native C++/CUDA Engine**: Direct memory access via a hardened C-Bridge for maximum performance.
- **Autonomous Weight Alignment**: Self-stabilizing neural weights through active geometric backpropagation.
- **Zero-Cloud Architecture**: 100% on-device. No telemetry. No images. Pure technical sincerity.

---

## 🛠️ Quick Start

### 1. The Workbench (UI)
```bash
cd client
npm install
npm run dev
```

### 2. The Neural Engine (Core)
```bash
# Requires CUDA 12.0+
cd engine
./run_v12_swarm.bat
```

---

## 📚 Repository Structure

- `engine/`: C++ native engine core (The 1.5M Param Core).
- `client/`: Professional Next.js 15 workbench and landing page.
- `results/`: [Detailed Bencharks](results/BENCHMARKS.md) (10k TPS / 11MB VRAM).
- `docs/`: Technical deep-dives into engine logic and architecture.
- `DEMO.md`: [2-Minute Experience Guide](DEMO.md).

---

<div align="center">
  <p><i>Orchestrating the future of local intelligence, one agent at a time.</i></p>
  <p><b>Built with Technical Sincerity.</b></p>
</div>
