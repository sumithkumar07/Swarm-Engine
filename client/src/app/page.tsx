"use client";

import React, { useState, useEffect } from 'react';
import { 
  Terminal, Shield, Cpu, Globe, Activity, 
  Zap, Database, Layers, Network,
  ChevronRight, BrainCircuit, Fingerprint,
  Play, CheckCircle2, AlertCircle, RefreshCw
} from 'lucide-react';
import InteractionFeed from '@/components/InteractionFeed';

export default function Home() {
  const [protocolStage, setProtocolStage] = useState(0); // 0-4
  const [booting, setBooting] = useState(false);

  const startBoot = () => {
    setBooting(true);
    setTimeout(() => {
      setProtocolStage(1);
      setBooting(false);
    }, 2000);
  };

  const steps = [
    { name: 'Protocol Initialization', desc: 'Secure boot sequence' },
    { name: 'Cloud Sync', desc: 'Hugging Face Handshake' },
    { name: 'Swarm Allocation', desc: 'Node assignment' },
    { name: 'Active Manifold', desc: 'Neural interaction' },
    { name: 'Analysis', desc: 'Manifold diagnostics' }
  ];

  return (
    <div className="flex flex-col h-full">
      
      {/* 1. TOP HEADER - MiroFish Style Breadcrumbs */}
      <header className="h-20 bg-white border-b border-[var(--ws-border)] flex items-center justify-between px-10 sticky top-0 z-40">
        <div className="flex items-center space-x-4">
           {steps.map((step, i) => (
             <React.Fragment key={step.name}>
               <div className={`flex items-center space-x-3 transition-opacity duration-300 ${i <= protocolStage ? 'opacity-100' : 'opacity-30'}`}>
                  <div className={`step-node ${
                    i < protocolStage ? 'bg-[var(--miro-blue)] border-[var(--miro-blue)] text-white' : 
                    i === protocolStage ? 'border-[var(--miro-blue)] text-[var(--miro-blue)] animate-step' : 
                    'border-slate-300 text-slate-400'
                  }`}>
                    {i < protocolStage ? <CheckCircle2 className="w-4 h-4" /> : i + 1}
                  </div>
                  <div className="hidden xl:block">
                     <p className="text-[12px] font-bold text-slate-800 leading-none mb-1">{step.name}</p>
                     <p className="text-[10px] text-slate-400 uppercase tracking-tighter">{step.desc}</p>
                  </div>
               </div>
               {i < steps.length - 1 && <ChevronRight className="w-4 h-4 text-slate-200 mx-2" />}
             </React.Fragment>
           ))}
        </div>

        <div className="flex items-center space-x-4">
           <div className="px-4 py-2 bg-slate-50 border border-slate-200 rounded-lg flex items-center space-x-3">
              <div className="w-2 h-2 rounded-full bg-emerald-500 shadow-[0_0_8px_rgba(16,185,129,0.5)]" />
              <span className="text-[11px] font-bold text-slate-600 uppercase tracking-widest">Protocol Active</span>
           </div>
           <button className="p-2 hover:bg-slate-100 rounded-full transition-colors text-slate-400">
              <RefreshCw className="w-4 h-4" />
           </button>
        </div>
      </header>

      {/* 2. MAIN WORKSPACE AREA */}
      <div className="flex-1 p-8 space-y-8 max-w-[1600px] mx-auto w-full">
        
        {/* BOOT STAGE (Step 0) */}
        {protocolStage === 0 && (
          <div className="miro-card p-20 flex flex-col items-center justify-center text-center space-y-6">
             <div className="w-20 h-20 bg-blue-50 rounded-3xl flex items-center justify-center text-[var(--miro-blue)]">
                <BrainCircuit className="w-10 h-10" />
             </div>
             <div className="max-w-md">
                <h2 className="text-2xl font-bold text-slate-900 mb-2">Initialize Titan Protocol</h2>
                <p className="text-slate-500 text-sm">Synchronizing local neural weights (1,532,472 parameters) with the global Sovereign manifold.</p>
             </div>
             <button 
                onClick={startBoot}
                disabled={booting}
                className="px-8 py-3 bg-[var(--miro-blue)] hover:bg-[var(--miro-blue-hover)] text-white rounded-xl font-bold transition-all shadow-lg shadow-blue-500/20 active:scale-95 flex items-center space-x-2"
             >
                {booting ? <Activity className="w-4 h-4 animate-spin" /> : <Play className="w-4 h-4" />}
                <span>{booting ? 'Initializing...' : 'Unlock Sovereign Manifold'}</span>
             </button>
          </div>
        )}

        {/* ACTIVE STAGES (Step 1+) */}
        {protocolStage >= 1 && (
          <div className="grid grid-cols-12 gap-8 animate-in fade-in slide-in-from-bottom-4 duration-700">
            
            {/* Left: Swarm Intelligence */}
            <div className="col-span-12 lg:col-span-3 space-y-6">
               <div className="miro-card p-6">
                  <div className="flex items-center justify-between mb-8">
                     <span className="text-[11px] font-bold text-slate-400 uppercase tracking-[0.2em]">Swarm Nodes</span>
                     <Network className="w-4 h-4 text-slate-300" />
                  </div>
                  <div className="space-y-4">
                     {['Alpha', 'Beta', 'Delta', 'Gamma'].map((id) => (
                       <div key={id} onClick={() => setProtocolStage(3)} className="group flex items-center justify-between p-4 rounded-xl border border-slate-100 hover:border-blue-200 hover:bg-blue-50/50 transition-all cursor-pointer">
                          <div className="flex items-center space-x-4">
                             <div className="w-10 h-10 rounded-lg bg-slate-50 flex items-center justify-center group-hover:bg-white transition-colors">
                                <Cpu className="w-5 h-5 text-slate-400 group-hover:text-[var(--miro-blue)]" />
                             </div>
                             <div>
                                <p className="text-xs font-bold text-slate-800">Node-{id}</p>
                                <p className="text-[10px] text-slate-400 uppercase font-bold tracking-tighter">1.5M Param Titan</p>
                             </div>
                          </div>
                          <div className="w-1.5 h-1.5 rounded-full bg-emerald-500" />
                       </div>
                     ))}
                  </div>
                  <div className="mt-8 p-4 bg-slate-50 rounded-xl border border-slate-100 italic text-[11px] text-slate-500 text-center">
                    "All nodes synchronized at locallab.sbs"
                  </div>
               </div>
            </div>

            {/* Center: The Core Interaction HUB */}
            <div className="col-span-12 lg:col-span-6 min-h-[700px]">
               <div className="miro-card h-full flex flex-col overflow-hidden">
                  <div className="px-6 py-4 border-b border-slate-100 flex items-center justify-between bg-white/50 backdrop-blur">
                     <div className="flex items-center space-x-3">
                        <Terminal className="w-4 h-4 text-slate-400" />
                        <h3 className="text-[11px] font-bold text-slate-800 uppercase tracking-widest">Sovereign Deployment Terminal</h3>
                     </div>
                     <span className="text-[10px] font-bold text-blue-600 bg-blue-50 px-3 py-1 rounded-full border border-blue-100">CLOUD_ACTIVE</span>
                  </div>
                  <div className="flex-1 overflow-hidden relative">
                     <InteractionFeed />
                  </div>
               </div>
            </div>

            {/* Right: Analytics & Diagnostics */}
            <div className="col-span-12 lg:col-span-3 space-y-6">
               <div className="miro-card p-6">
                  <div className="flex items-center justify-between mb-8">
                     <span className="text-[11px] font-bold text-slate-400 uppercase tracking-[0.2em]">Architecture Telemetry</span>
                     <Activity className="w-4 h-4 text-slate-300" />
                  </div>
                  <div className="space-y-6">
                     <div className="space-y-2">
                        <div className="flex justify-between text-[11px] font-bold">
                           <span className="text-slate-500">Manifold Synchronization</span>
                           <span className="text-[var(--miro-blue)] text-xs">98.4%</span>
                        </div>
                        <div className="h-1.5 bg-slate-100 rounded-full overflow-hidden">
                           <div className="h-full bg-[var(--miro-blue)] w-[98%] rounded-full shadow-[0_0_5px_rgba(0,111,238,0.3)]" />
                        </div>
                     </div>

                     <div className="grid grid-cols-1 gap-4">
                        <div className="p-4 bg-slate-50 rounded-xl border border-slate-100 space-y-1">
                           <p className="text-[10px] text-slate-400 uppercase font-bold tracking-widest">Unit Latency</p>
                           <p className="text-xl font-bold text-slate-800 data-text">0.42<span className="text-xs text-slate-400 ml-1">ms</span></p>
                        </div>
                        <div className="p-4 bg-slate-50 rounded-xl border border-slate-100 space-y-1">
                           <p className="text-[10px] text-slate-400 uppercase font-bold tracking-widest">Neural VRAM</p>
                           <p className="text-xl font-bold text-slate-800 data-text">1.54<span className="text-xs text-slate-400 ml-1">GB</span></p>
                        </div>
                     </div>

                     <div className="pt-4 border-t border-slate-100">
                        <div className="flex items-center space-x-3 p-3 bg-blue-50/50 rounded-lg border border-blue-100/50">
                           <AlertCircle className="w-4 h-4 text-blue-500" />
                           <p className="text-[10px] text-blue-600 font-medium tracking-tight">Sovereign 1.5M Param Engine detected. Optimized for CUDA Local-Link.</p>
                        </div>
                     </div>
                  </div>
               </div>

               <div className="miro-card p-6 bg-slate-900 group relative overflow-hidden">
                  <div className="absolute top-[-20%] right-[-10%] w-40 h-40 bg-blue-500/10 rounded-full blur-[60px] " />
                  <div className="relative z-10 space-y-4">
                     <p className="text-[10px] font-bold text-zinc-500 uppercase tracking-widest">System Entropy</p>
                     <div className="flex items-end space-x-1 h-12">
                        {Array.from({ length: 24 }).map((_, i) => (
                          <div key={i} className="flex-1 bg-blue-500/40 rounded-t-sm" style={{ height: `${Math.sin(i / 3) * 50 + 50}%` }} />
                        ))}
                     </div>
                     <p className="text-[9px] font-mono text-zinc-600">Sync: NOMINAL_PROTOCOL_LOCKED</p>
                  </div>
               </div>
            </div>

          </div>
        )}
      </div>

    </div>
  );
}
