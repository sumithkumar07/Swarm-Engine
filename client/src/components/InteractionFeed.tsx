"use client";

import React, { useState, useEffect, useRef } from 'react';
import { Terminal, Cpu, Shield, Globe, Send, Binary, Activity, Layers } from 'lucide-react';

// --- CONFIGURATION ---
const HF_SPACE_URL = "https://sumith007-sovereign-brain.hf.space"; 

interface Message {
  id: string;
  sender: string;
  role: 'agent' | 'system';
  content: string;
  timestamp: string;
  color: string;
}

const AGENT_MAP: Record<string, { color: string, name: string, glow: string }> = {
  "Alpha": { color: "text-blue-400", name: "TITAN-ALPHA", glow: "shadow-blue-500/20" },
  "Beta": { color: "text-emerald-400", name: "TITAN-BETA", glow: "shadow-emerald-500/20" },
  "Delta": { color: "text-purple-400", name: "TITAN-DELTA", glow: "shadow-purple-500/20" },
  "Gamma": { color: "text-amber-400", name: "TITAN-GAMMA", glow: "shadow-amber-500/20" }
};

export default function InteractionFeed() {
  const [messages, setMessages] = useState<Message[]>([]);
  const [isLive, setIsLive] = useState(false);
  const [isStreaming, setIsStreaming] = useState(false);
  const scrollRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    setMessages([
      {
        id: '0',
        sender: 'CORE',
        role: 'system',
        content: `Initialization Protocol: Sovereign v1.51 | Synchronizing Global Manifold...`,
        timestamp: new Date().toLocaleTimeString(),
        color: 'text-zinc-500'
      }
    ]);

    fetch(`${HF_SPACE_URL}/status`)
      .then(res => res.json())
      .then(data => {
        if(data.engine_loaded) {
          setIsLive(true);
          addSystemMessage("Neural Link Established. Titan-Nodes: [ALPHA, BETA, DELTA, GAMMA] Verified.");
        }
      })
      .catch(() => {
        addSystemMessage("[LATENCY_FAULT] Cloud Brain synchronization pending...");
      });
  }, []);

  const addSystemMessage = (text: string) => {
    setMessages(prev => [...prev, {
      id: Math.random().toString(),
      sender: 'SYSTEM',
      role: 'system',
      content: text,
      timestamp: new Date().toLocaleTimeString(),
      color: 'text-zinc-600'
    }]);
  };

  const triggerAgentAction = async (agentId: string) => {
    if(!isLive || isStreaming) return;
    setIsStreaming(true);
    
    try {
      const res = await fetch(`${HF_SPACE_URL}/act`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ agent_id: agentId, max_chars: 60, temp: 0.6 })
      });
      const data = await res.json();
      
      setMessages(prev => [...prev, {
        id: Math.random().toString(),
        sender: AGENT_MAP[agentId].name,
        role: 'agent',
        content: data.content,
        timestamp: new Date().toLocaleTimeString(),
        color: AGENT_MAP[agentId].color
      }]);
    } catch (e) {
      addSystemMessage(`[ERROR] Execution failure in Node ${agentId}. Check Telemetry.`);
    } finally {
      setIsStreaming(false);
    }
  };

  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [messages]);

  return (
    <div className="flex flex-col h-full bg-[#020203]/40 border border-white/5 rounded-2xl overflow-hidden backdrop-blur-xl relative group shadow-2xl">
      <div className="glint-overlay" />
      
      {/* 1. TERMINAL HEADER */}
      <div className="flex items-center justify-between px-6 py-4 border-b border-white/5 bg-white/2">
        <div className="flex items-center space-x-3">
          <Terminal className="w-4 h-4 text-blue-500" />
          <span className="text-[10px] font-bold tracking-[0.2em] text-zinc-400 uppercase">Interaction_Terminal</span>
        </div>
        <div className="flex items-center space-x-4">
          <div className="flex items-center space-x-2 px-3 py-1 bg-white/5 rounded-full border border-white/5">
             <div className={`w-1.5 h-1.5 rounded-full ${isLive ? 'bg-emerald-500 shadow-[0_0_8px_#10b981]' : 'bg-red-500'} animate-pulse`} />
             <span className="text-[9px] font-bold text-zinc-500 tracking-widest">{isLive ? 'SYNCED' : 'IDLE'}</span>
          </div>
          <Binary className="w-4 h-4 text-zinc-700 hover:text-blue-500 transition-colors cursor-pointer" />
        </div>
      </div>

      {/* 2. NEURAL FEED AREA */}
      <div 
        ref={scrollRef}
        className="flex-1 overflow-y-auto p-6 space-y-5 data-text text-[13px] scroll-smooth"
      >
        {messages.map((msg) => (
          <div key={msg.id} className="flex flex-col space-y-2 group/msg translate-y-2 animate-in fade-in duration-500">
            <div className="flex items-center justify-between">
               <div className="flex items-center space-x-2">
                  <span className={`text-[9px] px-2 py-0.5 rounded border ${msg.role === 'system' ? 'text-zinc-600 border-zinc-800' : 'text-blue-400 border-blue-500/20 bg-blue-500/10'} font-bold tracking-widest`}>
                    {msg.sender}
                  </span>
                  <span className="text-[9px] text-zinc-700 tracking-tighter">[{msg.timestamp}]</span>
               </div>
               <div className="w-1 h-1 rounded-full bg-zinc-800 group-hover/msg:bg-blue-500/50 transition-colors" />
            </div>
            <div className={`pl-4 border-l border-white/5 py-1 ${msg.role === 'system' ? 'text-zinc-600 italic' : 'text-zinc-100'}`}>
              <span className={msg.role === 'agent' ? 'opacity-90' : 'opacity-60 text-xs'}>
                {msg.content}
              </span>
            </div>
          </div>
        ))}
        {isStreaming && (
          <div className="flex items-center space-x-2 text-blue-500/50 animate-pulse pl-4">
             <Activity className="w-3 h-3" />
             <span className="text-[10px] uppercase font-bold tracking-[0.2em]">Neural Thinking...</span>
          </div>
        )}
      </div>

      {/* 3. COMMAND CONSOLE */}
      <div className="p-6 border-t border-white/5 bg-white/2 flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <Layers className="w-4 h-4 text-zinc-800" />
          <span className="text-[9px] text-zinc-600 font-bold tracking-[0.2em] uppercase">Ready_Command</span>
        </div>
        
        <div className="flex items-center space-x-3">
          {Object.keys(AGENT_MAP).map(id => (
            <button 
              key={id}
              onClick={() => triggerAgentAction(id)}
              disabled={!isLive || isStreaming}
              title={`Trigger ${AGENT_MAP[id].name}`}
              className={`flex items-center space-x-2 px-4 py-2 rounded-xl border border-white/5 bg-white/5 hover:border-blue-500/30 hover:bg-blue-500/5 transition-all duration-300 group ${(!isLive || isStreaming) ? 'opacity-20 cursor-not-allowed' : 'hover:scale-105 active:scale-95'}`}
            >
              <Send className={`w-3 h-3 ${AGENT_MAP[id].color} group-hover:scale-110 transition-transform`} />
              <span className="text-[10px] font-bold text-zinc-500 group-hover:text-zinc-300">{id}</span>
            </button>
          ))}
        </div>
      </div>

      {/* 4. SCANLINE OVERLAY */}
      <div className="absolute inset-0 pointer-events-none opacity-[0.02] bg-[linear-gradient(rgba(18,16,16,0)_50%,rgba(0,0,0,0.25)_50%),linear-gradient(90deg,rgba(255,0,0,0.06),rgba(0,255,0,0.02),rgba(0,0,255,0.06))] bg-[length:100%_2px,3px_100%]" />
    </div>
  );
}
