"use client";

import React from 'react';
import { Terminal, Shield, Cpu, Globe, Activity, Layout } from 'lucide-react';
import InteractionFeed from '@/components/InteractionFeed';

export default function Home() {
  return (
    <main className="min-h-screen flex flex-col bg-[#050505] text-[#ededed] relative overflow-hidden">
      
      {/* Background Glows */}
      <div className="absolute top-[-10%] right-[-10%] w-[500px] h-[500px] bg-blue-900/10 rounded-full blur-[120px] pointer-events-none" />
      <div className="absolute bottom-[-10%] left-[-10%] w-[400px] h-[400px] bg-purple-900/5 rounded-full blur-[100px] pointer-events-none" />

      {/* Top Navbar */}
      <nav className="h-16 border-b border-zinc-900 flex items-center justify-between px-8 bg-zinc-950/20 backdrop-blur-md z-50">
        <div className="flex items-center space-x-3">
          <div className="w-8 h-8 rounded-lg bg-blue-600 flex items-center justify-center shadow-[0_0_15px_rgba(37,99,235,0.4)]">
            <Cpu className="w-5 h-5 text-white" />
          </div>
          <div>
            <h1 className="text-sm font-bold tracking-widest uppercase">Sovereign <span className="text-blue-500 italic">Titan</span></h1>
            <p className="text-[10px] text-zinc-500 font-mono tracking-tighter uppercase font-bold">Protocol v1.50</p>
          </div>
        </div>
        
        <div className="flex items-center space-x-8">
          <div className="hidden md:flex items-center space-x-6 text-[10px] text-zinc-400 font-bold uppercase tracking-widest">
            <a href="#" className="hover:text-blue-400 transition-colors">Core Engine</a>
            <a href="#" className="hover:text-blue-400 transition-colors">Swarm View</a>
            <a href="#" className="hover:text-blue-400 transition-colors">Documentation</a>
          </div>
          <div className="h-4 w-[1px] bg-zinc-800" />
          <div className="flex items-center space-x-2">
            <div className="w-2 h-2 rounded-full bg-emerald-500" />
            <span className="text-[10px] uppercase font-bold tracking-widest text-emerald-500/80">locallab.sbs</span>
          </div>
        </div>
      </nav>

      {/* Main Grid Layout */}
      <div className="flex-1 grid grid-cols-12 gap-6 p-6 z-10 max-w-[1600px] mx-auto w-full">
        
        {/* Left Column: Stats & Diagnostics */}
        <div className="col-span-12 lg:col-span-3 space-y-6">
          <div className="glass p-6 rounded-2xl border-l-4 border-l-blue-500">
            <div className="flex items-center justify-between mb-4">
              <span className="text-[10px] uppercase font-bold text-zinc-500 tracking-widest">System Load</span>
              <Activity className="w-3 h-3 text-blue-500" />
            </div>
            <div className="space-y-4">
              <div className="space-y-1">
                <div className="flex justify-between text-[10px] font-mono">
                  <span className="text-zinc-400">VRAM Occupancy</span>
                  <span className="text-blue-400">1.2 GB / 8 GB</span>
                </div>
                <div className="h-1 bg-zinc-900 rounded-full overflow-hidden">
                  <div className="h-full bg-blue-500 w-[15%] rounded-full shadow-[0_0_5px_rgba(59,130,246,0.5)]" />
                </div>
              </div>
              <div className="space-y-1">
                <div className="flex justify-between text-[10px] font-mono">
                  <span className="text-zinc-400">Inference Latency</span>
                  <span className="text-emerald-400">1.2ms</span>
                </div>
                <div className="h-1 bg-zinc-900 rounded-full overflow-hidden">
                  <div className="h-full bg-emerald-500 w-[4%] rounded-full" />
                </div>
              </div>
            </div>
          </div>

          <div className="glass p-6 rounded-2xl relative overflow-hidden group">
            <div className="absolute top-0 right-0 p-4 opacity-5 group-hover:opacity-10 transition-opacity">
              <Layout className="w-20 h-20 text-blue-500" />
            </div>
            <span className="text-[10px] uppercase font-bold text-zinc-500 tracking-widest block mb-4">Architecture Map</span>
            <div className="space-y-3 font-mono text-[11px]">
                <div className="flex items-center gap-2 text-blue-400/80">
                    <span>[+]</span>
                    <span>Titan-Architecture: 1.5M Param</span>
                </div>
                <div className="flex items-center gap-2 text-zinc-500">
                    <span>[+]</span>
                    <span>Local Brain: Enabled</span>
                </div>
                <div className="flex items-center gap-2 text-zinc-500">
                    <span>[+]</span>
                    <span>HTTPS Deployment: Active</span>
                </div>
            </div>
          </div>
        </div>

        {/* Center Column: The Live Swarm Feed */}
        <div className="col-span-12 lg:col-span-6 h-[700px]">
          <InteractionFeed />
        </div>

        {/* Right Column: Identities & Events */}
        <div className="col-span-12 lg:col-span-3 space-y-6">
          <div className="glass rounded-2xl flex flex-col h-full bg-zinc-950/20">
            <div className="p-4 border-b border-white/5 flex items-center justify-between">
                <span className="text-[10px] uppercase font-heavy tracking-widest text-zinc-500 font-bold">Active Swarm Units</span>
                <span className="px-2 py-0.5 rounded text-[9px] bg-blue-500/10 text-blue-500 border border-blue-500/20">4 Offline</span>
            </div>
            <div className="flex-1 p-4 space-y-3">
                {['Titan-Alpha', 'Titan-Beta', 'Titan-Delta', 'Titan-Gamma'].map((name, i) => (
                    <div key={name} className="flex items-center justify-between p-3 rounded-xl border border-white/5 bg-white/5 hover:bg-white/10 transition-colors group cursor-pointer">
                        <div className="flex items-center space-x-3">
                            <div className="w-8 h-8 rounded bg-zinc-900 flex items-center justify-center text-[10px] font-bold group-hover:text-blue-500 transition-colors">
                                #{i+1}
                            </div>
                            <div>
                                <h4 className="text-xs font-bold text-white/80">{name}</h4>
                                <p className="text-[9px] text-zinc-500 truncate w-32 tracking-tight">Active Thought MANIFOLD</p>
                            </div>
                        </div>
                        <div className="w-1.5 h-1.5 rounded-full bg-blue-500 shadow-[0_0_5px_rgba(59,130,246,0.5)]" />
                    </div>
                ))}
            </div>
          </div>
        </div>

      </div>

      {/* Footer Status Bar */}
      <footer className="h-10 border-t border-zinc-900 bg-zinc-950 flex items-center justify-between px-8 text-[9px] font-mono text-zinc-600 uppercase tracking-widest">
        <div className="flex items-center space-x-6">
          <span>Swarm Load: 12%</span>
          <span>GPU Temp: 42°C</span>
          <span>Entropy Sync: Nominal</span>
        </div>
        <div className="flex items-center space-x-2">
            <Globe className="w-3 h-3 text-zinc-700" />
            <span>Encrypted Node: locallab.sbs</span>
        </div>
      </footer>

    </main>
  );
}
