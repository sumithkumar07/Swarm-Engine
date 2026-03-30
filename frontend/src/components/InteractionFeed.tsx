"use client";

import React, { useState, useEffect, useRef } from 'react';
import { Terminal, Cpu, Shield, AlertCircle } from 'lucide-react';

interface Message {
  id: string;
  sender: string;
  role: 'agent' | 'system';
  content: string;
  timestamp: string;
  color: string;
}

const MOCK_INITIAL_MESSAGES: Message[] = [
  {
    id: '1',
    sender: 'SYSTEM',
    role: 'system',
    content: 'Initializing Sovereign Architecture...',
    timestamp: '16:01:22',
    color: 'text-zinc-500'
  },
  {
    id: '2',
    sender: 'TITAN-ALPHA',
    role: 'agent',
    content: 'VRAM mapping complete. Parameter synchronization at 100%. Hello, Sovereign.',
    timestamp: '16:01:24',
    color: 'text-blue-400'
  },
  {
    id: '3',
    sender: 'TITAN-BETA',
    role: 'agent',
    content: 'Neural manifold aligned. Monitoring social timeline for incoming entropy signals.',
    timestamp: '16:01:25',
    color: 'text-emerald-400'
  },
  {
    id: '4',
    sender: 'SYSTEM',
    role: 'system',
    content: '[EVENT] High-density message burst detected in Swarm Layer.',
    timestamp: '16:01:30',
    color: 'text-amber-500'
  }
];

export default function InteractionFeed() {
  const [messages, setMessages] = useState<Message[]>(MOCK_INITIAL_MESSAGES);
  const scrollRef = useRef<HTMLDivElement>(null);

  // Auto-scroll to bottom
  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [messages]);

  return (
    <div className="flex flex-col h-full bg-[#0a0a0a] border border-zinc-800 rounded-xl overflow-hidden glass relative group">
      
      {/* Feed Header */}
      <div className="flex items-center justify-between px-4 py-3 border-b border-zinc-900 bg-zinc-950/50">
        <div className="flex items-center space-x-2">
          <Terminal className="w-4 h-4 text-blue-500" />
          <span className="text-xs font-mono font-bold tracking-widest text-zinc-400 uppercase">Swarm Interaction Terminal</span>
        </div>
        <div className="flex items-center space-x-3">
          <div className="flex items-center space-x-1.5">
            <div className="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse" />
            <span className="text-[10px] text-emerald-500/80 font-mono font-bold uppercase tracking-tighter">Live Connection</span>
          </div>
          <Shield className="w-3.5 h-3.5 text-zinc-700 hover:text-blue-500 transition-colors cursor-pointer" />
        </div>
      </div>

      {/* Messages Area */}
      <div 
        ref={scrollRef}
        className="flex-1 overflow-y-auto p-4 space-y-4 font-mono text-sm scrollbar-thin scrollbar-thumb-zinc-800"
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

      {/* Feed Footer / Input Simulation */}
      <div className="p-3 border-t border-zinc-900 bg-zinc-950/30 flex items-center space-x-3">
        <Cpu className="w-4 h-4 text-zinc-700 animate-spin-slow" />
        <div className="flex-1 h-6 bg-zinc-900/50 rounded flex items-center px-3 border border-zinc-800/50">
          <span className="text-[10px] text-zinc-700 font-bold animate-pulse">Waiting for Swarm Input_</span>
        </div>
      </div>

      {/* Aesthetic Scanline Overlay */}
      <div className="absolute inset-0 pointer-events-none opacity-[0.03] bg-[linear-gradient(rgba(18,16,16,0)_50%,rgba(0,0,0,0.25)_50%),linear-gradient(90deg,rgba(255,0,0,0.06),rgba(0,255,0,0.02),rgba(0,0,255,0.06))] bg-[length:100%_2px,3px_100%]" />
    </div>
  );
}
