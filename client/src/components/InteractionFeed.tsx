"use client";

import React, { useState, useEffect, useRef } from 'react';
import { Terminal, Cpu, Shield, Globe, Send } from 'lucide-react';

// --- CONFIGURATION ---
// Once you create your HF Space, replace this URL with your custom HF Space URL
const HF_SPACE_URL = "https://sumith007-sovereign-brain.hf.space"; 

interface Message {
  id: string;
  sender: string;
  role: 'agent' | 'system';
  content: string;
  timestamp: string;
  color: string;
}

const AGENT_MAP: Record<string, { color: string, name: string }> = {
  "Alpha": { color: "text-blue-400", name: "TITAN-ALPHA" },
  "Beta": { color: "text-emerald-400", name: "TITAN-BETA" },
  "Delta": { color: "text-purple-400", name: "TITAN-DELTA" },
  "Gamma": { color: "text-amber-400", name: "TITAN-GAMMA" }
};

export default function InteractionFeed() {
  const [messages, setMessages] = useState<Message[]>([]);
  const [isLive, setIsLive] = useState(false);
  const scrollRef = useRef<HTMLDivElement>(null);

  // Initialize with System Logs
  useEffect(() => {
    setMessages([
      {
        id: '0',
        sender: 'SYSTEM',
        role: 'system',
        content: `Sovereign Client linked to locallab.sbs. Connecting to Cloud Brain...`,
        timestamp: new Date().toLocaleTimeString(),
        color: 'text-zinc-500'
      }
    ]);

    // Check Cloud Status
    fetch(`${HF_SPACE_URL}/status`)
      .then(res => res.json())
      .then(data => {
        if(data.engine_loaded) {
          setIsLive(true);
          addSystemMessage("Cloud Brain Synchronization: SUCCESS.");
        }
      })
      .catch(() => {
        addSystemMessage("[OFFLINE] Waiting for Hugging Face Space deployment...");
      });
  }, []);

  const addSystemMessage = (text: string) => {
    setMessages(prev => [...prev, {
      id: Math.random().toString(),
      sender: 'SYSTEM',
      role: 'system',
      content: text,
      timestamp: new Date().toLocaleTimeString(),
      color: 'text-zinc-500'
    }]);
  };

  const triggerAgentAction = async (agentId: string) => {
    if(!isLive) return;
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
      addSystemMessage(`[ERROR] Communication failure with ${agentId}.`);
    }
  };

  // Auto-scroll logic
  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [messages]);

  return (
    <div className="flex flex-col h-full bg-[#0a0a0a] border border-zinc-800 rounded-xl overflow-hidden glass relative group shadow-2xl">
      
      {/* Feed Header */}
      <div className="flex items-center justify-between px-4 py-3 border-b border-zinc-900 bg-zinc-950/50">
        <div className="flex items-center space-x-2">
          <Terminal className="w-4 h-4 text-blue-500" />
          <span className="text-xs font-mono font-bold tracking-widest text-zinc-400 uppercase">Swarm Interaction Terminal</span>
        </div>
        <div className="flex items-center space-x-3">
          <div className="flex items-center space-x-1.5">
            <div className={`w-1.5 h-1.5 rounded-full ${isLive ? 'bg-emerald-500 animate-pulse' : 'bg-red-500'}`} />
            <span className={`text-[10px] font-mono font-bold uppercase tracking-tighter ${isLive ? 'text-emerald-500/80' : 'text-red-500/80'}`}>
              {isLive ? 'Cloud Link: Active' : 'Cloud Link: Idle'}
            </span>
          </div>
          <Shield className="w-3.5 h-3.5 text-zinc-700 hover:text-blue-500 transition-colors cursor-pointer" />
        </div>
      </div>

      {/* Messages Area */}
      <div 
        ref={scrollRef}
        className="flex-1 overflow-y-auto p-4 space-y-4 font-mono text-sm scrollbar-thin scrollbar-thumb-zinc-800 selection:bg-blue-500/20"
      >
        {messages.map((msg) => (
          <div key={msg.id} className="flex flex-col space-y-1 group/msg">
            <div className="flex items-center space-x-2">
              <span className="text-[10px] text-zinc-600 font-bold whitespace-nowrap">[{msg.timestamp}]</span>
              <span className={`text-[10px] px-1.5 py-0.5 rounded-sm bg-zinc-900 border border-zinc-800 font-bold tracking-tight ${msg.color}`}>
                {msg.sender}
              </span>
            </div>
            <div className={`pl-2 border-l border-zinc-900 group-hover/msg:border-zinc-700 transition-colors py-0.5 mt-0.5 ${msg.role === 'system' ? 'text-zinc-500 italic' : 'text-zinc-200'}`}>
              {msg.content}
            </div>
          </div>
        ))}
      </div>

      {/* Feed Footer / Command Console */}
      <div className="p-3 border-t border-zinc-900 bg-zinc-950/30 flex items-center justify-between">
        <div className="flex items-center space-x-2">
          <Cpu className="w-4 h-4 text-zinc-700 animate-spin-slow" />
          <span className="text-[10px] text-zinc-700 font-bold">READY_</span>
        </div>
        
        <div className="flex items-center space-x-2">
          <span className="text-[10px] text-zinc-500 font-bold uppercase mr-2">Trigger Thought:</span>
          {Object.keys(AGENT_MAP).map(id => (
            <button 
              key={id}
              onClick={() => triggerAgentAction(id)}
              disabled={!isLive}
              className={`p-1.5 rounded border border-zinc-800 hover:border-blue-500/50 bg-zinc-900 transition-all ${!isLive ? 'opacity-30' : 'hover:scale-105 active:scale-95'}`}
            >
              <Send className={`w-3 h-3 ${AGENT_MAP[id].color}`} />
            </button>
          ))}
        </div>
      </div>

      {/* Aesthetic Scanline Overlay */}
      <div className="absolute inset-0 pointer-events-none opacity-[0.03] bg-[linear-gradient(rgba(18,16,16,0)_50%,rgba(0,0,0,0.25)_50%),linear-gradient(90deg,rgba(255,0,0,0.06),rgba(0,255,0,0.02),rgba(0,0,255,0.06))] bg-[length:100%_2px,3px_100%]" />
    </div>
  );
}
