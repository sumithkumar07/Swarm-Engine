"use client";

import React, { useState, useEffect } from 'react';
import { 
  Terminal, Shield, Cpu, Globe, Activity, 
  Layout, Zap, Database, Layers, Network,
  ChevronRight, BrainCircuit, Fingerprint
} from 'lucide-react';
import InteractionFeed from '@/components/InteractionFeed';

export default function Home() {
  const [protocolStage, setProtocolStage] = useState(2); // 0: Boot, 1: Sync, 2: Swarm, 3: Analysis

  return (
    <main className="min-h-screen flex flex-col relative overflow-hidden">
      
      {/* 1. TOP PROTOCOL NAVIGATION (MiroFish Style) */}
      <nav className="h-16 border-b border-white/5 flex items-center justify-between px-8 bg-[#020203]/80 backdrop-blur-2xl z-50">
        <div className="flex items-center space-x-6">
          <div className="flex items-center space-x-3">
             <div className="w-9 h-9 rounded-xl bg-blue-600/20 border border-blue-500/30 flex items-center justify-center shadow-[0_0_20px_rgba(59,130,246,0.2)]">
                <BrainCircuit className="w-5 h-5 text-blue-400" />
             </div>
             <div>
                <h1 className="text-sm font-bold tracking-[0.2em] uppercase text-white">Sovereign <span className="text-blue-500">Titan</span></h1>
                <p className="text-[9px] text-zinc-500 font-mono tracking-tighter uppercase font-bold">1.5M Param / Node</p>
             </div>
          </div>

          <div className="hidden lg:flex items-center space-x-2 ml-8">
            {['Protocol Boot', 'HF-Neural Sync', 'Sovereign Swarm', 'Manifold Analysis'].map((step, i) => (
              <React.Fragment key={step}>
                <div className={`flex items-center space-x-2 px-3 py-1 rounded-full text-[10px] font-bold uppercase tracking-widest transition-all duration-500 ${
                  i <= protocolStage ? 'text-blue-400 bg-blue-500/10' : 'text-zinc-600'
                }`}>
                  <span className={`w-1 h-1 rounded-full ${i <= protocolStage ? 'bg-blue-400 shadow-[0_0_5px_#3b82f6]' : 'bg-zinc-700'}`} />
                  <span>{step}</span>
                </div>
                {i < 3 && <ChevronRight className="w-3 h-3 text-zinc-800" />}
              </React.Fragment>
            ))}
          </div>
        </div>

        <div className="flex items-center space-x-6">
           <div className="flex items-center space-x-2 text-[10px] font-bold text-emerald-500/80 bg-emerald-500/5 px-3 py-1.5 rounded-lg border border-emerald-500/20">
              <div className="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse" />
              <span className="tracking-widest">LOCALLAB.SBS / LIVE</span>
           </div>
        </div>
      </nav>

      {/* 2. THE 3-PANEL COMMAND CONSOLE */}
      <div className="flex-1 grid grid-cols-12 gap-6 p-6 z-10 max-w-[1800px] mx-auto w-full">
        
        {/* PANEL 1: SWARM TOPOLOGY (Left) */}
        <div className="col-span-12 lg:col-span-3 space-y-6">
          <div className="titan-card h-[400px] flex flex-col p-6">
            <div className="glint-overlay" />
            <div className="flex items-center justify-between mb-8">
              <div className="space-y-1">
                <h3 className="text-[10px] uppercase font-bold text-zinc-500 tracking-[0.2em]">Swarm Topology</h3>
                <p className="text-xs text-white/50 font-medium">4 Active Titan Nodes</p>
              </div>
              <Network className="w-4 h-4 text-blue-500" />
            </div>

            {/* SVG Topology Visualizer */}
            <div className="flex-1 relative flex items-center justify-center">
               <svg className="w-full h-full" viewBox="0 0 200 200">
                  {/* Neural Grid Lines */}
                  <circle cx="100" cy="100" r="40" fill="none" stroke="rgba(59,130,246,0.1)" strokeWidth="1" strokeDasharray="4" />
                  <circle cx="100" cy="100" r="70" fill="none" stroke="rgba(59,130,246,0.05)" strokeWidth="1" />
                  
                  {/* Connections */}
                  <line x1="100" y1="50" x2="100" y2="150" stroke="rgba(59,130,246,0.1)" strokeWidth="0.5" />
                  <line x1="50" y1="100" x2="150" y2="100" stroke="rgba(59,130,246,0.1)" strokeWidth="0.5" />

                  {/* Nodes */}
                  {[
                    { id: 'Alpha', x: 100, y: 40 },
                    { id: 'Beta', x: 160, y: 100 },
                    { id: 'Delta', x: 100, y: 160 },
                    { id: 'Gamma', x: 40, y: 100 }
                  ].map((node) => (
                    <g key={node.id} className="cursor-pointer group">
                      <circle 
                        cx={node.x} cy={node.y} r="8" 
                        className="fill-blue-500/20 stroke-blue-500/50 group-hover:fill-blue-500/40 group-hover:stroke-blue-400 transition-all duration-300"
                        strokeWidth="1"
                      />
                      <circle cx={node.x} cy={node.y} r="12" className="fill-blue-500/5 animate-pulse" />
                      <text x={node.x} y={node.y - 12} textAnchor="middle" className="text-[8px] fill-zinc-500 font-bold uppercase tracking-tighter">{node.id}</text>
                    </g>
                  ))}
               </svg>
            </div>

            <div className="mt-4 pt-4 border-t border-white/5 grid grid-cols-2 gap-4">
               <div>
                  <p className="text-[9px] text-zinc-600 uppercase font-bold tracking-widest">Load</p>
                  <p className="text-xs text-blue-400 data-text">12.4%</p>
               </div>
               <div>
                  <p className="text-[9px] text-zinc-600 uppercase font-bold tracking-widest">Stability</p>
                  <p className="text-xs text-emerald-400 data-text">LOCKED</p>
               </div>
            </div>
          </div>

          <div className="titan-card p-6">
            <div className="glint-overlay" />
            <h3 className="text-[10px] uppercase font-bold text-zinc-500 tracking-[0.2em] mb-6">Security Context</h3>
            <div className="space-y-4">
               {[
                 { label: 'Neural Encryption', value: 'AES-256', icon: Shield },
                 { label: 'Access Protocol', value: 'DirectLink', icon: Zap },
                 { label: 'Authentication', value: 'Sovereign-ID', icon: Fingerprint }
               ].map((item) => (
                 <div key={item.label} className="flex items-center justify-between group">
                    <div className="flex items-center space-x-3">
                       <item.icon className="w-3.5 h-3.5 text-zinc-600 group-hover:text-blue-500 transition-colors" />
                       <span className="text-[10px] text-zinc-400 group-hover:text-zinc-200 transition-colors">{item.label}</span>
                    </div>
                    <span className="text-[10px] text-blue-500/70 font-mono">{item.value}</span>
                 </div>
               ))}
            </div>
          </div>
        </div>

        {/* PANEL 2: INTERACTION FEED (Center) */}
        <div className="col-span-12 lg:col-span-6 h-[750px]">
          <InteractionFeed />
        </div>

        {/* PANEL 3: TITAN TELEMETRY (Right) */}
        <div className="col-span-12 lg:col-span-3 space-y-6">
          <div className="titan-card p-6 flex flex-col">
            <div className="glint-overlay" />
            <div className="flex items-center justify-between mb-8">
               <h3 className="text-[10px] uppercase font-bold text-zinc-500 tracking-[0.2em]">Titan Telemetry</h3>
               <Activity className="w-4 h-4 text-purple-500" />
            </div>

            <div className="space-y-8 flex-1">
               <div className="space-y-3">
                  <div className="flex justify-between items-end">
                     <span className="text-[10px] text-zinc-500 uppercase font-bold">Inference Density</span>
                     <span className="text-[14px] text-zinc-100 data-text">1,532,472 <span className="text-[10px] text-zinc-600 italic">params</span></span>
                  </div>
                  <div className="h-1.5 bg-zinc-900 rounded-full overflow-hidden">
                     <div className="h-full bg-gradient-to-r from-blue-600 to-purple-600 w-[78%] rounded-full opacity-80" />
                  </div>
               </div>

               <div className="grid grid-cols-2 gap-6">
                  <div className="space-y-2 p-3 rounded-xl bg-white/5 border border-white/5">
                     <p className="text-[9px] text-zinc-500 uppercase font-bold">VRAM Usage</p>
                     <p className="text-lg text-white data-text">1.2<span className="text-[10px] text-zinc-600 ml-1">GB</span></p>
                  </div>
                  <div className="space-y-2 p-3 rounded-xl bg-white/5 border border-white/5">
                     <p className="text-[9px] text-zinc-500 uppercase font-bold">Avg Latency</p>
                     <p className="text-lg text-white data-text">0.4<span className="text-[10px] text-zinc-600 ml-1">ms</span></p>
                  </div>
               </div>

               <div className="p-4 rounded-xl border border-blue-500/10 bg-blue-500/5 space-y-3">
                  <div className="flex items-center space-x-2 text-[10px] text-blue-400 font-bold uppercase tracking-widest">
                     <Layers className="w-3 h-3" />
                     <span>Manifold Status</span>
                  </div>
                  <div className="flex items-center space-x-2">
                     <div className="status-pulse text-blue-500 bg-blue-500" />
                     <span className="text-[11px] font-mono text-zinc-300 uppercase tracking-tighter">Geometric Coherence: 0.998</span>
                  </div>
               </div>
            </div>

            <div className="mt-8">
               <div className="flex items-center justify-between text-[10px] text-zinc-600 font-bold uppercase mb-3">
                  <span>Engine Threads</span>
                  <span>48 Core Pool</span>
               </div>
               <div className="flex space-x-1">
                  {Array.from({ length: 24 }).map((_, i) => (
                    <div key={i} className={`flex-1 h-3 rounded-sm ${i < 18 ? 'bg-blue-500/40' : 'bg-zinc-800'}`} />
                  ))}
               </div>
            </div>
          </div>

          <div className="titan-card p-6 bg-purple-500/5">
            <div className="glint-overlay" />
            <h3 className="text-[10px] uppercase font-bold text-zinc-500 tracking-[0.2em] mb-4">Neural Buffer</h3>
            <div className="flex items-end space-x-1 h-12">
               {Array.from({ length: 30 }).map((_, i) => (
                  <div 
                    key={i} 
                    className="flex-1 bg-purple-500/20 rounded-t-sm" 
                    style={{ height: `${Math.random() * 100}%` }} 
                  />
               ))}
            </div>
            <p className="text-[9px] text-zinc-600 mt-3 font-mono">Entropy Syncing: ACTIVE_STREAM</p>
          </div>
        </div>
      </div>

      {/* 3. FOOTER STATUS BAR */}
      <footer className="h-12 border-t border-white/5 bg-[#020203] flex items-center justify-between px-8 text-[9px] font-mono text-zinc-600 uppercase tracking-[0.2em] z-50">
        <div className="flex items-center space-x-8">
          <div className="flex items-center space-x-2">
             <span className="w-1.5 h-1.5 rounded-full bg-emerald-500" />
             <span>Swarm Load: 12%</span>
          </div>
          <span>GPU Temp: 42°C</span>
          <span>Entropy: Nominal</span>
        </div>
        <div className="flex items-center space-x-3">
           <Database className="w-3 h-3 text-zinc-800" />
           <span className="text-zinc-700">Encrypted Node: locallab.sbs</span>
        </div>
      </footer>

    </main>
  );
}
