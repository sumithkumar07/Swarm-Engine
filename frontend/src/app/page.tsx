"use client";

import React from 'react';
import { Terminal, Shield,Cpu, Globe } from 'lucide-react';

export default function Home() {
  return (
    <main className="min-h-screen flex flex-col items-center justify-center relative overflow-hidden bg-[#050505] p-8">
      {/* Background Glow */}
      <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[600px] h-[600px] bg-blue-900/10 rounded-full blur-[120px] pointer-events-none" />

      {/* Content */}
      <div className="relative z-10 text-center max-w-4xl space-y-12">
        
        {/* Header Section */}
        <div className="space-y-4">
          <div className="inline-flex items-center space-x-2 px-3 py-1 rounded-full bg-blue-500/10 border border-blue-500/20 text-blue-400 text-xs font-mono uppercase tracking-widest mb-6 animate-pulse">
            <span className="w-2 h-2 rounded-full bg-blue-500" />
            <span>Titan Protocol: Active</span>
          </div>
          
          <h1 className="text-6xl md:text-8xl font-bold tracking-tighter text-white">
            LOCALLAB<span className="text-blue-500">.SBS</span>
          </h1>
          
          <p className="text-xl text-zinc-500 font-light tracking-wide max-w-2xl mx-auto text-balance">
            Deploying the world's most parameter-efficient multi-agent swarm. 
            Native C++ / CUDA Architecture.
          </p>
        </div>

        {/* Feature Grid */}
        <div className="grid grid-cols-1 md:grid-cols-3 gap-6 pt-12">
          <div className="glass p-8 rounded-2xl text-left space-y-4 hover:border-blue-500/40 transition-colors duration-500 group">
            <Terminal className="w-8 h-8 text-blue-500" />
            <h3 className="text-lg font-semibold text-white/90">Autonomous Swarm</h3>
            <p className="text-sm text-zinc-500 leading-relaxed">
              1.5 Million Parameter agents living persistently in VRAM for zero-latency thought emission.
            </p>
          </div>
          
          <div className="glass p-8 rounded-2xl text-left space-y-4 hover:border-blue-500/40 transition-colors duration-500 group">
            <Shield className="w-8 h-8 text-blue-500" />
            <h3 className="text-lg font-semibold text-white/90">Native Offline</h3>
            <p className="text-sm text-zinc-500 leading-relaxed">
              No APIs. No cloud dependencies. Pure local execution at the speed of your silicon hardware.
            </p>
          </div>

          <div className="glass p-8 rounded-2xl text-left space-y-4 hover:border-blue-500/40 transition-colors duration-500 group">
            <Cpu className="w-8 h-8 text-blue-500" />
            <h3 className="text-lg font-semibold text-white/90">CUDA-Accelerated</h3>
            <p className="text-sm text-zinc-500 leading-relaxed">
              Parallelized neural gates designed for NVIDIA GeForce architecture.
            </p>
          </div>
        </div>

        {/* Footer / Status */}
        <div className="pt-16 flex flex-col items-center space-y-4">
          <div className="h-[1px] w-48 bg-zinc-800" />
          <div className="text-[10px] uppercase tracking-[0.3em] text-zinc-600 font-bold flex items-center gap-2">
            <Globe className="w-3 h-3" />
            System Initializing on locallab.sbs
          </div>
        </div>

      </div>
    </main>
  );
}
