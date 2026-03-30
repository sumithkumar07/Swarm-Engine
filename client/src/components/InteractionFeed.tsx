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

const AGENT_MAP: Record<string, { color: string, name: string }> = {
  "Alpha": { color: "text-[var(--miro-blue)]", name: "TITAN-ALPHA" },
  "Beta": { color: "text-emerald-500", name: "TITAN-BETA" },
  "Delta": { color: "text-purple-500", name: "TITAN-DELTA" },
  "Gamma": { color: "text-amber-500", name: "TITAN-GAMMA" }
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
        sender: 'PROTOCOL',
        role: 'system',
        content: `Initialization sequence ready. Awaiting Sovereign Manifold authorization...`,
        timestamp: new Date().toLocaleTimeString(),
        color: 'text-slate-400'
      }
    ]);

    fetch(`${HF_SPACE_URL}/status`)
      .then(res => res.json())
      .then(data => {
        if(data.engine_loaded) {
          setIsLive(true);
        }
      })
      .catch(() => {});
  }, []);

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
      // Silent fail for clean SaaS feel
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
    <div className="flex flex-col h-full bg-white relative overflow-hidden">
      
      {/* Messages Area - Clean SaaS View */}
      <div 
        ref={scrollRef}
        className="flex-1 overflow-y-auto p-8 space-y-6 data-text text-[13px] leading-relaxed"
      >
        {messages.map((msg) => (
          <div key={msg.id} className="flex flex-col space-y-2 group/msg animate-in fade-in slide-in-from-left-2 duration-300">
            <div className="flex items-center space-x-3">
               <span className={`text-[10px] px-2 py-0.5 rounded-full border border-slate-100 font-bold tracking-widest ${msg.color}`}>
                 {msg.sender}
               </span>
               <span className="text-[9px] text-slate-300 font-bold uppercase tracking-tighter">{msg.timestamp}</span>
            </div>
            <div className={`pl-4 border-l-2 ${msg.role === 'system' ? 'border-slate-100 text-slate-400 italic' : 'border-blue-100 text-slate-700'} py-1`}>
              {msg.content}
            </div>
          </div>
        ))}
        {isStreaming && (
          <div className="flex items-center space-x-3 text-blue-500 animate-pulse pl-4">
             <div className="w-1.5 h-1.5 rounded-full bg-blue-500" />
             <span className="text-[10px] uppercase font-bold tracking-widest">Neural Manifold Thinking...</span>
          </div>
        )}
      </div>

      {/* Interaction Footer - Miro Style */}
      <div className="p-6 bg-slate-50 border-t border-slate-100 flex items-center justify-between">
        <div className="flex items-center space-x-2">
           <Activity className="w-4 h-4 text-slate-300" />
           <span className="text-[10px] text-slate-400 font-bold uppercase tracking-widest">Sovereign Link Ready</span>
        </div>
        
        <div className="flex items-center space-x-3">
          {Object.keys(AGENT_MAP).map(id => (
            <button 
              key={id}
              onClick={() => triggerAgentAction(id)}
              disabled={!isLive || isStreaming}
              className={`px-4 py-2 rounded-lg bg-white border border-slate-200 text-[11px] font-bold text-slate-600 hover:border-blue-500 hover:text-blue-600 transition-all shadow-sm ${(!isLive || isStreaming) ? 'opacity-20' : 'hover:-translate-y-0.5 active:translate-y-0'}`}
            >
              {id}
            </button>
          ))}
        </div>
      </div>
    </div>
  );
}
