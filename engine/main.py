import os
import ctypes
from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel
from typing import Optional
import threading
import time
import random
import json
import uuid
from collections import deque
import uvicorn

app = FastAPI(title="Swarm-Engine Intelligence API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# --- LOAD NATIVE C++ ENGINE ---
LIB_PATH = os.path.join(os.path.dirname(__file__), "libswarm.so")

# Fallback for Windows local testing
if os.name == 'nt':
    LIB_PATH = os.path.join(os.path.dirname(__file__), "swarm.dll")

class SwarmEngineAPI:
    def __init__(self):
        self.lib = None
        self.master_brain = None
        self.agents = {}
        
        try:
            if not os.path.exists(LIB_PATH):
                print(f"[ERROR] Binary not found at {LIB_PATH}")
                return

            self.lib = ctypes.CDLL(LIB_PATH)
            
            # --- STRICT FUNCTION SIGNATURES (64-BIT COMPATIBLE) ---
            
            # Brain Factory
            self.lib.swarm_init_master.argtypes = []
            self.lib.swarm_init_master.restype = ctypes.c_void_p
            
            # Agent Factory
            self.lib.swarm_init_agent.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_int]
            self.lib.swarm_init_agent.restype = ctypes.c_void_p
            
            # Observation
            self.lib.swarm_agent_observe.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
            self.lib.swarm_agent_observe.restype = None
            
            # Action/Inference
            self.lib.swarm_agent_act.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_double]
            self.lib.swarm_agent_act.restype = ctypes.c_char_p
            
            # Initialize Global Brain
            self.master_brain = self.lib.swarm_init_master()
            
            if not self.master_brain:
                print("[ERROR] Master Brain allocation failed (Memory?).")
                return

            # Hardcoded 20-Agent Core (Fail-safe for Cloud)
            CORE_PERSONALITIES = [
                {"name": "Aura", "personality": "Calm, philosophical, and deeply analytical."},
                {"name": "Vortex", "personality": "Aggressive, high-energy, and disruptive."},
                {"name": "Sage", "personality": "Cautious, historical, and skeptical."},
                {"name": "Echo", "personality": "Concise, mimicry-prone, and adaptive."},
                {"name": "Pulse", "personality": "Obsessed with metrics, latency, and raw data."},
                {"name": "Zenith", "personality": "Optimistic, visionary, and proactive."},
                {"name": "Nadir", "personality": "Pessimistic, cautionary, and defensive."},
                {"name": "Flux", "personality": "Unpredictable, lateral-thinking, and creative."},
                {"name": "Core", "personality": "Structural, logical, and rigid."},
                {"name": "Drift", "personality": "Quiet, observant, and subtly influential."},
                {"name": "Vector", "personality": "Direct, goal-oriented, and decisive."},
                {"name": "Shadow", "personality": "Mysterious, questioning, and quiet."},
                {"name": "Glow", "personality": "Inspiring, warm, and highly social."},
                {"name": "Frost", "personality": "Cold, detached, and purely logical."},
                {"name": "Spark", "personality": "Quick-tempered, reactive, and fast."},
                {"name": "Stone", "personality": "Slow, deliberate, and stubborn."},
                {"name": "Bloom", "personality": "Growth-oriented, expansive, and positive."},
                {"name": "Void", "personality": "Nihilistic, reductive, and quiet."},
                {"name": "Prism", "personality": "Multi-faceted, complex, and verbose."},
                {"name": "Axis", "personality": "Balance-seeking, mediating, and fair."}
            ]

            # Initialize 20 Core Agents (to prevent Cloud OOM)
            try:
                # Try loading from file first if available
                personalities_path = os.path.join(os.path.dirname(__file__), "personalities.json")
                if os.path.exists(personalities_path):
                    with open(personalities_path, 'r') as f:
                        all_p = json.load(f)
                        active_p = all_p[:20]
                else:
                    active_p = CORE_PERSONALITIES
                
                print(f"[ENGINE] Loading {len(active_p)} unique agent personalities...")
                for i, p in enumerate(active_p):
                    name = p["name"]
                    persona = p["personality"]
                    ptr = self.lib.swarm_init_agent(name.encode('utf-8'), self.master_brain, 1000 * (i+1))
                    if ptr:
                        self.agents[name] = ptr
                        obs = f"My name is {name}. My role is: {persona}"
                        self.lib.swarm_agent_observe(ptr, obs.encode('utf-8'))
            except Exception as pe:
                print(f"[ERROR] Personalities failed: {pe}. Using emergency Alpha-Delta fallback.")
                for i, name in enumerate(["Alpha", "Beta", "Delta", "Gamma"]):
                    ptr = self.lib.swarm_init_agent(name.encode('utf-8'), self.master_brain, 100 * (i+1))
                    if ptr: self.agents[name] = ptr
            
            print(f"[SUCCESS] Swarm-Engine Loaded: {len(self.agents)} agents active.")
        except Exception as e:
            print(f"[ERROR] Critical Bridge Failure: {e}")
            self.lib = None

    def __del__(self):
        """Explicitly free C++ pointers to prevent memory leaks."""
        if not self.lib:
            return
        
        print("[CLEANUP] Freeing 100 Agent States...")
        for name, ptr in self.agents.items():
            self.lib.swarm_free_agent(ptr)
        
        if self.master_brain:
            print("[CLEANUP] Deallocating 1.5M Parameter Neural Core...")
            self.lib.swarm_free_master(self.master_brain)

# Global Engine Instance
engine = SwarmEngineAPI()

# --- SWARM HEARTBEAT STATE ---
swarm_active = False
current_session_id = None
pulse_buffer = deque(maxlen=50)
heartbeat_thread = None
swarm_lock = threading.Lock()

def log_to_session(session_id, pulse):
    os.makedirs("sessions", exist_ok=True)
    filename = f"sessions/session_{session_id}.json"
    try:
        data = []
        if os.path.exists(filename):
            with open(filename, 'r') as f:
                try:
                    data = json.load(f)
                except:
                    data = []
        data.append(pulse)
        with open(filename, 'w') as f:
            json.dump(data, f, indent=2)
    except Exception as e:
        print(f"[ERROR] Session logging failed: {e}")

def swarm_logic_loop():
    global swarm_active, pulse_buffer, current_session_id
    
    pulse_count = 0
    agent_names = list(engine.agents.keys())
    
    if not agent_names:
        print("[ERROR] No agents available for swarm loop.")
        swarm_active = False
        return

    print(f"[SWARM] Starting autonomous swarm pulse for session {current_session_id}")
    
    while swarm_active:
        pulse_count += 1
        
        # --- ROUND-ROBIN SPEAKER SELECTION WITH INTERRUPTION ---
        # Base: Rotate through agents in order
        speaker_idx = (pulse_count - 1) % len(agent_names)
        
        # 20% Chance of an "interruption" or "lateral response" from a random agent
        if random.random() < 0.2:
            speaker_idx = random.randint(0, len(agent_names) - 1)
            
        speaker_name = agent_names[speaker_idx]
        speaker_ptr = engine.agents[speaker_name]
        
        try:
            with swarm_lock:
                # 1. Agent Generates Action (Thinking)
                # Scaled for 3.2M model context: temp 0.7 for creativity
                raw_res = engine.lib.swarm_agent_act(speaker_ptr, 40, 0.7)
                text = raw_res.decode('utf-8', errors='ignore') if raw_res else "... [Neural Silence] ..."
                
                # 2. Broadcast to ALL other agents (Cross-Observation)
                broadcast_msg = f"[{speaker_name}]: {text}"
                for name, ptr in engine.agents.items():
                    if name != speaker_name:
                        engine.lib.swarm_agent_observe(ptr, broadcast_msg.encode('utf-8'))
            
            # Create heartbeat pulse
            pulse = {
                "session_id": current_session_id,
                "pulse_index": pulse_count,
                "timestamp": time.time(),
                "speaker": speaker_name,
                "content": text,
                "agent_id": f"Agent_{agent_names.index(speaker_name)+1:03d}"
            }
            
            pulse_buffer.append(pulse)
            log_to_session(current_session_id, pulse)
            print(f"[PULSE] {speaker_name}: {text[:40]}...")
            
        except Exception as e:
            print(f"[HEARTBEAT ERROR] {e}")
            
        # Delay between pulses (faster for 100 agents to keep it interesting)
        time.sleep(2.5)

class StartSwarmRequest(BaseModel):
    seed_text: str

class ObserveRequest(BaseModel):
    agent_id: str
    text: str

class ActRequest(BaseModel):
    agent_id: str
    max_chars: Optional[int] = 50
    temp: Optional[float] = 0.5

@app.get("/")
async def root():
    return {
        "status": "online",
        "engine": "v1.51 Stabilized",
        "ready_agents": list(engine.agents.keys())
    }

@app.get("/status")
async def get_status():
    return {
        "engine_loaded": engine.lib is not None and engine.master_brain is not None,
        "agents_ready": list(engine.agents.keys())
    }

@app.post("/observe")
async def observe(req: ObserveRequest):
    if not engine.lib or req.agent_id not in engine.agents:
        return {"error": "Engine offline"}
    
    try:
        with swarm_lock:
            agent_ptr = engine.agents[req.agent_id]
            engine.lib.swarm_agent_observe(agent_ptr, req.text.encode('utf-8'))
        return {"status": "ok"}
    except:
        return {"error": "Observation crash"}

@app.post("/act")
async def act(req: ActRequest):
    if not engine.lib or req.agent_id not in engine.agents:
        return {"error": "Engine offline"}
    
    try:
        with swarm_lock:
            agent_ptr = engine.agents[req.agent_id]
            # Call into C++
            raw_res = engine.lib.swarm_agent_act(agent_ptr, req.max_chars, req.temp)
        
        # Decode with safety
        if raw_res:
            content = raw_res.decode('utf-8', errors='ignore')
        else:
            content = "... [Neural Silence] ..."
            
        return {
            "agent": req.agent_id,
            "content": content
        }
    except Exception as e:
        print(f"[CRASH] /act endpoint: {e}")
        return {"error": "Neural Inference Crash"}

# --- NEW SWARM ENDPOINTS ---

@app.post("/swarm/start")
async def start_swarm(req: StartSwarmRequest):
    global swarm_active, heartbeat_thread
    
    if swarm_active:
        return {"status": "already_active"}
    
    global current_session_id
    current_session_id = str(uuid.uuid4())[:8]
    swarm_active = True
    
    # Initialize session file
    log_to_session(current_session_id, {"type": "init", "seed": req.seed_text, "timestamp": time.time()})
    
    heartbeat_thread = threading.Thread(target=swarm_logic_loop, args=(req.seed_text,), daemon=True)
    heartbeat_thread.start()
    
    return {"status": "swarm_initiated", "seed": req.seed_text, "session_id": current_session_id}

@app.post("/swarm/stop")
async def stop_swarm():
    global swarm_active
    swarm_active = False
    return {"status": "swarm_stopped"}

@app.get("/swarm/agents")
async def get_swarm_agents():
    """Returns the list of all agents and their personalities for the UI legend."""
    try:
        with open(os.path.join(os.path.dirname(__file__), "personalities.json"), 'r') as f:
            return json.load(f)
    except:
        # Fallback if file not found
        return [{"name": name, "personality": "Standard Neural Agent"} for name in engine.agents.keys()]

@app.get("/swarm/pulses")
async def get_pulses():
    return {
        "active": swarm_active,
        "pulses": list(pulse_buffer)
    }

@app.get("/swarm/session/{session_id}")
async def get_session_history(session_id: str):
    filename = f"sessions/session_{session_id}.json"
    if not os.path.exists(filename):
        return {"error": "Session not found"}
    try:
        with open(filename, 'r') as f:
            data = json.load(f)
        return {"session_id": session_id, "pulses": data}
    except:
        return {"error": "Read failure"}

@app.get("/swarm/report/{session_id}")
async def generate_swarm_report(session_id: str):
    filename = f"sessions/session_{session_id}.json"
    if not os.path.exists(filename):
        return {"error": "Session not found"}
    try:
        with open(filename, 'r') as f:
            data = json.load(f)
            
        seed_text = "Unknown"
        agent_counts = {}
        total_pulses = 0
        highlights = []
        
        for p in data:
            if p.get("type") == "init":
                seed_text = p.get("seed", "")
            elif "agent_id" in p:
                agent = p.get("agent_id").upper()
                agent_counts[agent] = agent_counts.get(agent, 0) + 1
                total_pulses += 1
                
                # Keep some logic fragments for the report
                content = p.get("content", "")
                if len(content) > 10 and len(highlights) < 5:
                    highlights.append(f"**AGENT-{agent}**: \"{content[:150]}...\"")
                    
        # Generate Markdown
        md = f"""# Swarm-Engine Analytical Report

## 1. Executive Summary
The Swarm-Engine C++ core successfully maintained an autonomous multi-agent simulation to resolve the initial prompt.

**Seed Reality:** *"{seed_text}"*

---

## 2. Telemetry & Engagement
- **Session UUID:** `{session_id}`
- **Total Neural Pulses:** {total_pulses}
- **Active Participating Agents:** {len(agent_counts.keys())}

### Participation Distribution:
"""
        for agent, count in sorted(agent_counts.items(), key=lambda item: item[1], reverse=True):
            md += f"- **AGENT-{agent}**: {count} pulses\n"
            
        md += """
---

## 3. Key Neural Fragments
*Extracted telemetry from the highest fidelity interaction paths:*

"""
        for h in highlights:
            md += f"- {h}\n"
            
        md += "\n> *Report automatically generated by Swarm-Engine.*"
        
        return {
            "session_id": session_id,
            "markdown": md
        }
    except Exception as e:
        return {"error": f"Report generation failed: {e}"}

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=7860)
