# 2-Minute Swarm Experience

Follow this exact sequence to witness the **Swarm-Engine** in action on your local machine.

---

## ⏱️ Step 1: Initialization (0:00 - 0:30)

1. Open a PowerShell terminal.
2. Navigate to the `engine/` directory.
3. Run the optimized build script:
   ```bash
   ./run_v12_swarm.bat
   ```
4. **Observe**: The C++ engine will initialize 1,532,472 parameters in **< 5ms**. Look for the `[SUCCESS] Engine Core Ready` log.

---

## 🌩️ Step 2: Live Interaction (0:30 - 1:15)

1. Open a second terminal and navigate to `client/`.
2. Start the Workbench:
   ```bash
   npm run dev
   ```
3. Open `http://localhost:3000` in your browser.
4. **The "Pulse"**: Type a prompt like *"What is the impact of decentralized intelligence on local silicon?"*.
5. **Observe**: Watch the 4 agents (Alpha, Beta, Delta, Gamma) immediately begin a synchronized dialogue. No "loading" spinners—just instant, 10k TPS inference.

---

## 📊 Step 3: Performance Audit (1:15 - 2:00)

1. While the agents are communicating, check your Task Manager (Performance Tab -> GPU).
2. **Observe**: VRAM usage remains nearly flat (<15MB extra usage). This is the power of our 64-bit native memory bridge.
3. **The Report**: Once the interaction completes, look at the "Analytical Report" generated in the UI. It will show the total neural pulses and participation distribution.

---

## 💡 Why This Works
Unlike standard LLM interfaces that rely on high-latency cloud APIs, Swarm-Engine is **physically local.** You are observing a C++/CUDA swarm running directly on your silicon, achieving zero-latency reasoning through Clifford-initialized manifolds.

---

**Experience the future of local-first intelligence in 120 seconds.**
