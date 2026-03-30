"use client";

import React, { useState, useEffect, useRef } from 'react';
import { 
  Rocket, ArrowRight, BrainCircuit, Cpu, 
  Network, Shield, Send, Activity, Fingerprint,
  Zap, Globe, ChevronRight, CheckCircle2, MessageSquare
} from 'lucide-react';

const HF_SPACE_URL = "https://sumith007-sovereign-brain.hf.space";

interface Message {
  id: string;
  sender: string;
  content: string;
  timestamp: string;
}

export default function Home() {
  const [view, setView] = useState<'landing' | 'grid'>('landing');
  const [messages, setMessages] = useState<Message[]>([]);
  const [input, setInput] = useState('');
  const [isLive, setIsLive] = useState(false);
  const [activeNode, setActiveNode] = useState<string | null>(null);
  const scrollRef = useRef<HTMLDivElement>(null);

  // Sync with Cloud
  useEffect(() => {
    fetch(`${HF_SPACE_URL}/status`)
      .then(res => res.json())
      .then(data => data.engine_loaded && setIsLive(true))
      .catch(() => {});
  }, []);

  const triggerSwarm = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!input.trim() || !isLive) return;

    const userMsg = { id: Date.now().toString(), sender: 'USER', content: input, timestamp: new Date().toLocaleTimeString() };
    setMessages(prev => [...prev, userMsg]);
    setInput('');

    // Trigger all 4 agents
    const agents = ['Alpha', 'Beta', 'Delta', 'Gamma'];
    for (const agentId of agents) {
      setActiveNode(agentId);
      try {
        const res = await fetch(`${HF_SPACE_URL}/act`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ agent_id: agentId, max_chars: 80, temp: 0.7 })
        });
        const data = await res.json();
        setMessages(prev => [...prev, {
          id: Math.random().toString(),
          sender: `AGENT-${agentId.toUpperCase()}`,
          content: data.content,
          timestamp: new Date().toLocaleTimeString()
        }]);
      } catch (e) {}
      await new Promise(r => setTimeout(r, 500)); // Staggered response
    }
    setActiveNode(null);
  };

  /* --- VIEW 1: SIMPLE SAAS LANDING --- */
  if (view === 'landing') {
    return (
      <div className="min-h-screen bg-white text-slate-900 flex flex-col overflow-y-auto">
        {/* Header */}
        <header className="px-12 py-8 flex justify-between items-center max-w-7xl mx-auto w-full">
           <div className="flex items-center space-x-3">
              <BrainCircuit className="w-8 h-8 text-[#0d6efd]" />
              <span className="font-bold text-xl tracking-tight uppercase">Sovereign</span>
           </div>
           <nav className="flex items-center space-x-8 text-sm font-semibold text-slate-500">
              <a href="#" className="hover:text-slate-900 transition-colors">Technology</a>
              <a href="#" className="hover:text-slate-900 transition-colors">Repository</a>
              <button 
                onClick={() => setView('grid')}
                className="btn-primary"
              >Launch Swarm</button>
           </nav>
        </header>

        {/* Hero */}
        <main className="flex-1 flex flex-col items-center justify-center text-center px-6 py-20 animate-hero">
            <h1 className="text-6xl md:text-8xl title-main mb-8 tracking-tight">
               The first autonomous <br />
               <span className="text-[#0d6efd]">Local Neural Swarm.</span>
            </h1>
            <p className="max-w-2xl text-xl text-slate-500 mb-12">
               Experience 1,532,472 parameters of local intelligence across 4 persistent agents. 
               Zero latency, 100% private, silicon-grade autonomy.
            </p>
            <div className="flex border-2 border-slate-100 p-2 rounded-2xl bg-white shadow-xl">
               <button 
                 onClick={() => setView('grid')}
                 className="px-10 py-5 bg-[#0d6efd] hover:bg-[#0b5ed7] text-white rounded-xl font-bold flex items-center space-x-3 transition-all"
               >
                  <span>Start Live Session</span>
                  <ArrowRight className="w-5 h-5" />
               </button>
            </div>
            <div className="mt-12 flex items-center space-x-8 opacity-40 grayscale">
               <div className="flex items-center space-x-2"><Cpu className="w-5 h-5" /> <span className="text-xs font-bold uppercase">C++ Native</span></div>
               <div className="flex items-center space-x-2"><Network className="w-5 h-5" /> <span className="text-xs font-bold uppercase">Quad-Node</span></div>
               <div className="flex items-center space-x-2"><Shield className="w-5 h-5" /> <span className="text-xs font-bold uppercase">Hardened</span></div>
            </div>
        </main>
      </div>
    );
  }

  /* --- VIEW 2: INTERACTIVE GRID SPACE --- */
  return (
    <div className="agent-canvas overflow-hidden flex flex-col">
      
      {/* Top Header Grid Area */}
      <div className="p-10 flex justify-between items-start pointer-events-none relative z-10">
         <div>
            <h2 className="title-main text-2xl text-slate-900">Titan Swarm Hub</h2>
            <p className="text-xs text-slate-400 font-bold tracking-widest uppercase">Protocol: Active_Manifold_01</p>
         </div>
         <button 
           onClick={() => setView('landing')}
           className="pointer-events-auto px-4 py-2 bg-white border border-slate-200 rounded-lg text-xs font-bold hover:bg-slate-50 transition-colors"
         >Exit Session</button>
      </div>

      {/* THE 4 AGENT GRID */}
      <div className="flex-1 grid grid-cols-2 gap-8 px-20 py-10 max-w-6xl mx-auto w-full relative z-10">
         {['Alpha', 'Beta', 'Delta', 'Gamma'].map((id) => (
           <div key={id} className={`agent-node flex flex-col justify-between ${activeNode === id ? 'thinking' : ''}`}>
              <div className="flex justify-between items-center mb-4">
                 <div className="flex items-center space-x-3">
                    <div className={`w-10 h-10 rounded-xl bg-slate-50 flex items-center justify-center border border-slate-100 ${activeNode === id ? 'bg-blue-50 border-blue-200' : ''}`}>
                       <Fingerprint className={`w-5 h-5 ${activeNode === id ? 'text-[#0d6efd]' : 'text-slate-300'}`} />
                    </div>
                    <div>
                       <p className="text-sm font-bold text-slate-800">Titan-{id}</p>
                       <p className="text-[10px] text-slate-400 font-bold uppercase tracking-tighter">Status: {activeNode === id ? 'Active' : 'Standby'}</p>
                    </div>
                 </div>
                 <div className={`w-2 h-2 rounded-full ${isLive ? 'bg-emerald-500' : 'bg-red-500'} ${activeNode === id ? 'animate-ping' : ''}`} />
              </div>
              <div className="flex-1 bg-slate-50/50 rounded-xl p-4 overflow-y-auto scrollbar-none">
                 <div className="space-y-3">
                    {messages.filter(m => m.sender === `AGENT-${id.toUpperCase()}`).slice(-1).map(m => (
                      <p key={m.id} className="text-xs text-slate-600 leading-relaxed italic animate-in fade-in duration-500">
                        "{m.content}"
                      </p>
                    ))}
                    {messages.filter(m => m.sender === `AGENT-${id.toUpperCase()}`).length === 0 && (
                      <p className="text-[10px] text-slate-300 uppercase tracking-widest text-center mt-8">Awaiting Manifold Pulse...</p>
                    )}
                 </div>
              </div>
           </div>
         ))}
      </div>

      {/* FLOATING COMMAND BAR */}
      <div className="command-bar">
         <form onSubmit={triggerSwarm} className="glass-bar flex items-center space-x-4">
            <div className="w-10 h-10 rounded-xl bg-slate-50 flex items-center justify-center border border-slate-100">
               <MessageSquare className="w-5 h-5 text-slate-400" />
            </div>
            <input 
               value={input}
               onChange={(e) => setInput(e.target.value)}
               placeholder="Enter collective command to the swarm..."
               className="flex-1 bg-transparent border-none outline-none text-sm text-slate-800 font-medium placeholder:text-slate-300"
            />
            <button 
               type="submit"
               disabled={!isLive || activeNode !== null}
               className="btn-primary flex items-center space-x-2 disabled:opacity-20"
            >
               <span>Launch Swarm</span>
               <Send className="w-4 h-4" />
            </button>
         </form>
         <div className="mt-4 flex justify-center space-x-8 text-[10px] text-slate-400 font-bold tracking-widest uppercase">
            <div className="flex items-center space-x-2"><div className="w-1 h-1 rounded-full bg-blue-400" /> <span>Local 1.5M Param</span></div>
            <div className="flex items-center space-x-2"><div className="w-1 h-1 rounded-full bg-emerald-400" /> <span>HuggingFace Link: Active</span></div>
         </div>
      </div>

    </div>
  );
}
