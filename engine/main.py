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

app = FastAPI(title="Sovereign Swarm Intelligence API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# --- LOAD NATIVE C++ ENGINE ---
LIB_PATH = os.path.join(os.path.dirname(__file__), "libsovereign.so")

# Fallback for Windows local testing
if os.name == 'nt':
    LIB_PATH = os.path.join(os.path.dirname(__file__), "sovereign.dll")

class SovereignAPI:
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
            self.lib.sovereign_init_master.argtypes = []
            self.lib.sovereign_init_master.restype = ctypes.c_void_p
            
            # Agent Factory
            self.lib.sovereign_init_agent.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_int]
            self.lib.sovereign_init_agent.restype = ctypes.c_void_p
            
            # Observation
            self.lib.sovereign_agent_observe.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
            self.lib.sovereign_agent_observe.restype = None
            
            # Action/Inference
            self.lib.sovereign_agent_act.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_double]
            self.lib.sovereign_agent_act.restype = ctypes.c_char_p
            
            # Initialize Global Brain
            self.master_brain = self.lib.sovereign_init_master()
            
            if not self.master_brain:
                print("[ERROR] Master Brain allocation failed (Memory?).")
                return

            # Initialize Agents
            agent_names = ["Alpha", "Beta", "Delta", "Gamma"]
            for i, name in enumerate(agent_names):
                ptr = self.lib.sovereign_init_agent(name.encode('utf-8'), self.master_brain, 100 * (i+1))
                if ptr:
                    self.agents[name] = ptr
            
            print(f"[SUCCESS] Sovereign Swarm Engine Loaded: {len(self.agents)} agents active.")
        except Exception as e:
            print(f"[ERROR] Critical Bridge Failure: {e}")
            self.lib = None

# Global Engine Instance
engine = SovereignAPI()

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
                data = json.load(f)
        data.append(pulse)
        with open(filename, 'w') as f:
            json.dump(data, f, indent=2)
    except Exception as e:
        print(f"[REPORTER ERROR] Persistent Log failed: {e}")

def swarm_logic_loop(seed_text: str):
    global swarm_active, pulse_buffer
    
    print(f"[SWARM] Heartbeat started with seed: {seed_text[:30]}...")
    
    # 1. Initial Broadcast
    for name, ptr in engine.agents.items():
        engine.lib.sovereign_agent_observe(ptr, seed_text.encode('utf-8'))
    
    agent_names = list(engine.agents.keys())
    
    while swarm_active:
        with swarm_lock:
            if not engine.lib: break
            
            # 2. Select Agent and Act
            speaker_name = random.choice(agent_names)
            speaker_ptr = engine.agents[speaker_name]
            
            try:
                raw_res = engine.lib.sovereign_agent_act(speaker_ptr, 80, 0.7)
                content = raw_res.decode('utf-8', errors='ignore') if raw_res else "... [Neural Silence] ..."
                
                pulse = {
                    "agent_id": speaker_name,
                    "content": content,
                    "timestamp": time.time()
                }
                pulse_buffer.append(pulse)
                if current_session_id:
                    log_to_session(current_session_id, pulse)
                
                # 3. Cross-Observation (The "Swarm" Sync)
                for name, ptr in engine.agents.items():
                    if name != speaker_name:
                        engine.lib.sovereign_agent_observe(ptr, f"[{speaker_name}]: {content}".encode('utf-8'))
                
                print(f"[PULSE] {speaker_name}: {content[:40]}...")
            except Exception as e:
                print(f"[HEARTBEAT CRASH] {e}")

        # 4. Neural Cooldown
        time.sleep(random.uniform(3.0, 6.0))

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
        agent_ptr = engine.agents[req.agent_id]
        engine.lib.sovereign_agent_observe(agent_ptr, req.text.encode('utf-8'))
        return {"status": "ok"}
    except:
        return {"error": "Observation crash"}

@app.post("/act")
async def act(req: ActRequest):
    if not engine.lib or req.agent_id not in engine.agents:
        return {"error": "Engine offline"}
    
    try:
        agent_ptr = engine.agents[req.agent_id]
        # Call into C++
        raw_res = engine.lib.sovereign_agent_act(agent_ptr, req.max_chars, req.temp)
        
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
                    highlights.append(f"**TITAN-{agent}**: \"{content[:150]}...\"")
                    
        # Generate Markdown
        md = f"""# Sovereign Swarm Analytical Report

## 1. Executive Summary
The Sovereign C++ engine successfully maintained an autonomous multi-agent simulation to resolve the injected reality seed.

**Seed Reality:** *"{seed_text}"*

---

## 2. Telemetry & Engagement
- **Session UUID:** `{session_id}`
- **Total Neural Pulses:** {total_pulses}
- **Active Participating Titans:** {len(agent_counts.keys())}

### Participation Distribution:
"""
        for agent, count in sorted(agent_counts.items(), key=lambda item: item[1], reverse=True):
            md += f"- **TITAN-{agent}**: {count} pulses\n"
            
        md += """
---

## 3. Key Neural Fragments
*Extracted telemetry from the highest fidelity interaction paths:*

"""
        for h in highlights:
            md += f"- {h}\n"
            
        md += "\n> *Report automatically generated by Sovereign Analytical Engine.*"
        
        return {
            "session_id": session_id,
            "markdown": md
        }
    except Exception as e:
        return {"error": f"Report generation failed: {e}"}

if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=7860)
