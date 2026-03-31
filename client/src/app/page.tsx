"use client";

import React, { useState, useEffect, useRef } from 'react';
import * as d3 from 'd3';
import { 
  Rocket, ArrowRight, BrainCircuit, Cpu, 
  Network, Shield, Send, Activity, Fingerprint,
  Zap, Globe, ChevronRight, CheckCircle2, MessageSquare,
  X, Info, Database, Clock, Maximize2, Minus, Plus
} from 'lucide-react';

const BACKEND_URL = "http://localhost:7860";

interface Message {
  id: string;
  sender: string;
  content: string;
  timestamp: string;
}

interface Node extends d3.SimulationNodeDatum {
  id: string;
  label: string;
  type: 'core' | 'agent';
  color?: string;
  uuid?: string;
}

interface Link extends d3.SimulationLinkDatum<Node> {
  id: string;
  label: string;
  source: string | Node;
  target: string | Node;
}

const Typewriter = ({ text }: { text: string }) => {
  const [displayText, setDisplayText] = useState('');
  useEffect(() => {
    let index = 0;
    setDisplayText('');
    const interval = setInterval(() => {
      index++;
      setDisplayText(text.slice(0, index));
      if (index >= text.length) clearInterval(interval);
    }, 15);
    return () => clearInterval(interval);
  }, [text]);
  return <span>{displayText}</span>;
}

export default function Home() {
  const [view, setView] = useState<'landing' | 'grid'>('landing');
  const [messages, setMessages] = useState<Message[]>([]);
  const [input, setInput] = useState('');
  const [isLive, setIsLive] = useState(false);
  const [selectedNode, setSelectedNode] = useState<Node | null>(null);
  const [showInspector, setShowInspector] = useState(false);
  const [sessionId, setSessionId] = useState<string | null>(null);
  const [showReport, setShowReport] = useState(false);
  const [reportContent, setReportContent] = useState('');
  const [hoveredNode, setHoveredNode] = useState<Node | null>(null);
  const [mousePos, setMousePos] = useState({ x: 0, y: 0 });
  
  const scrollRef = useRef<HTMLDivElement>(null);
  const svgRef = useRef<SVGSVGElement>(null);
  const zoomRef = useRef<any>(null);

  // --- D3 FORCE SIMULATION ---
  useEffect(() => {
    if (view !== 'grid' || !svgRef.current) return;

    const width = 1000;
    const height = 1000;
    const svg = d3.select(svgRef.current);
    svg.selectAll("*").remove(); // Clear for re-init

    // 1. Setup Data
    const nodes: Node[] = [
      { id: 'core', label: 'Core', type: 'core', uuid: 'MASTER_01' },
      { id: 'alpha', label: 'Titan-Alpha', type: 'agent', color: '#f97316', uuid: 'TITAN_A_7F2' },
      { id: 'beta', label: 'Titan-Beta', type: 'agent', color: '#ef4444', uuid: 'TITAN_B_9C4' },
      { id: 'delta', label: 'Titan-Delta', type: 'agent', color: '#3b82f6', uuid: 'TITAN_D_3E1' },
      { id: 'gamma', label: 'Titan-Gamma', type: 'agent', color: '#8b5cf6', uuid: 'TITAN_G_5B9' },
    ];

    const links: Link[] = [
      { id: 'l1', source: 'core', target: 'alpha', label: 'PULSE_SYNC' },
      { id: 'l2', source: 'core', target: 'beta', label: 'DATA_RELATION' },
      { id: 'l3', source: 'core', target: 'delta', label: 'NEURAL_LINK' },
      { id: 'l4', source: 'core', target: 'gamma', label: 'LOGIC_STREAM' },
      { id: 'm1', source: 'alpha', target: 'beta', label: 'MESH_PULSE' },
      { id: 'm2', source: 'beta', target: 'gamma', label: 'MESH_PULSE' },
      { id: 'm3', source: 'gamma', target: 'delta', label: 'MESH_PULSE' },
      { id: 'm4', source: 'delta', target: 'alpha', label: 'MESH_PULSE' },
    ];

    // 2. Setup Simulation
    const simulation = d3.forceSimulation<Node>(nodes)
      .force("link", d3.forceLink<Node, Link>(links).id(d => d.id).distance(250))
      .force("charge", d3.forceManyBody().strength(-2000))
      .force("center", d3.forceCenter(width / 2, height / 2))
      .force("collision", d3.forceCollide().radius(80));

    // 3. Zoom/Pan Setup
    const container = svg.append("g");
    const zoom = d3.zoom().on("zoom", (event) => {
      container.attr("transform", event.transform);
    });
    svg.call(zoom as any);
    zoomRef.current = zoom;

    // Deselect on background click
    svg.on("click", () => {
      setSelectedNode(null);
      setShowInspector(false);
    });

    // Arrowheads
    svg.append("defs").selectAll("marker")
      .data(["end"])
      .enter().append("marker")
      .attr("id", "arrowhead")
      .attr("viewBox", "0 0 10 10")
      .attr("refX", 25)
      .attr("refY", 5)
      .attr("markerWidth", 4)
      .attr("markerHeight", 4)
      .attr("orient", "auto")
      .append("path")
      .attr("d", "M 0 0 L 10 5 L 0 10 z")
      .attr("fill", "#cccccc");

    // Paths
    const link = container.append("g")
      .selectAll("path")
      .data(links)
      .enter().append("path")
      .attr("class", "pulse-path")
      .attr("id", d => `link-${d.id}`)
      .attr("marker-end", "url(#arrowhead)");

    // Labels on Paths
    const linkText = container.append("g")
      .selectAll("text")
      .data(links)
      .enter().append("text")
      .attr("class", "relationship-label")
      .append("textPath")
      .attr("xlink:href", d => `#link-${d.id}`)
      .attr("startOffset", "45%")
      .style("text-anchor", "middle")
      .text(d => d.label);

    // Nodes
    const node = container.append("g")
      .selectAll("g")
      .data(nodes)
      .enter().append("g")
      .on("click", (event, d) => {
        event.stopPropagation();
        setSelectedNode(d);
        setShowInspector(true);
        setHoveredNode(null);
      })
      .on("mouseover", (event, d) => {
         setHoveredNode(d);
         setMousePos({ x: event.clientX, y: event.clientY });
      })
      .on("mousemove", (event) => {
         setMousePos({ x: event.clientX, y: event.clientY });
      })
      .on("mouseout", () => {
         setHoveredNode(null);
      })
      .call(d3.drag<SVGGElement, Node>()
        .on("start", (event, d) => {
          if (!event.active) simulation.alphaTarget(0.3).restart();
          d.fx = d.x;
          d.fy = d.y;
        })
        .on("drag", (event, d) => {
          d.fx = event.x;
          d.fy = event.y;
        })
        .on("end", (event, d) => {
          if (!event.active) simulation.alphaTarget(0);
          d.fx = null;
          d.fy = null;
        }) as any);

    node.append("circle")
      .attr("r", d => d.type === 'core' ? 12 : 7)
      .attr("class", d => d.type === 'core' ? 'master-node-core' : `manifold-node node-${d.id}`);

    node.append("text")
      .attr("dy", d => d.type === 'core' ? 30 : -15)
      .attr("text-anchor", "middle")
      .attr("class", "text-[9px] font-black text-slate-400 uppercase tracking-widest")
      .text(d => d.label);

    // 4. Update Loop
    simulation.on("tick", () => {
      link.attr("d", (d: any) => {
        const dx = d.target.x - d.source.x;
        const dy = d.target.y - d.source.y;
        const dr = Math.sqrt(dx * dx + dy * dy) * 1.5; // Curvature
        return `M${d.source.x},${d.source.y}A${dr},${dr} 0 0,1 ${d.target.x},${d.target.y}`;
      });

      node.attr("transform", (d: any) => `translate(${d.x},${d.y})`);
    });

    return () => { simulation.stop(); };
  }, [view]);

  // Sync with Backend
  useEffect(() => {
    const checkStatus = () => {
      fetch(`${BACKEND_URL}/status`)
        .then(res => res.json())
        .then(data => data.engine_loaded && setIsLive(true))
        .catch(() => setIsLive(false));
    };
    checkStatus();
    const interval = setInterval(checkStatus, 5000);
    return () => clearInterval(interval);
  }, []);

  // Poll for Swarm Pulses
  useEffect(() => {
    let interval: NodeJS.Timeout;
    if (view === 'grid') {
      interval = setInterval(async () => {
        try {
          const res = await fetch(`${BACKEND_URL}/swarm/pulses`);
          const data = await res.json();
          if (data.pulses && data.pulses.length > 0) {
            setMessages(prev => {
              const existingIds = new Set(prev.map(m => m.id));
              const newMsgs = data.pulses
                .filter((p: any) => !existingIds.has(`${p.agent_id}-${p.timestamp}`))
                .map((p: any) => ({
                  id: `${p.agent_id}-${p.timestamp}`,
                  sender: `AGENT-${p.agent_id.toUpperCase()}`,
                  content: p.content,
                  timestamp: new Date(p.timestamp * 1000).toLocaleTimeString()
                }));
              return [...prev, ...newMsgs];
            });

            // Pulse Animation Trigger
            const latest = data.pulses[data.pulses.length - 1];
            d3.select(`.node-${latest.agent_id.toLowerCase()}`).classed('active', true);
            setTimeout(() => {
              d3.select(`.node-${latest.agent_id.toLowerCase()}`).classed('active', false);
            }, 1000);

            // Visual Data Packet (Network Traffic)
            const pulseLink = d3.selectAll('.pulse-path').filter((d: any) => d.target.id === latest.agent_id.toLowerCase() || d.source.id === latest.agent_id.toLowerCase());
            if (!pulseLink.empty()) {
                const linkNode = pulseLink.node() as SVGPathElement;
                if (linkNode && svgRef.current) {
                    const length = linkNode.getTotalLength();
                    const packet = d3.select(svgRef.current).select("g").append("circle")
                       .attr("r", 4)
                       .attr("fill", "#0d6efd")
                       .style("filter", "drop-shadow(0 0 6px #0d6efd)");
                       
                    packet.transition()
                       .duration(800)
                       .ease(d3.easeLinear)
                       .attrTween("transform", function() {
                          return function(t) {
                             const p = linkNode.getPointAtLength(t * length);
                             return `translate(${p.x},${p.y})`;
                          };
                       })
                       .on("end", () => packet.remove());
                }
            }
          }
        } catch (e) {}
      }, 2000);
    }
    return () => clearInterval(interval);
  }, [view]);

  const triggerSwarm = async (e: React.FormEvent) => {
    e.preventDefault();
    if (!input.trim() || !isLive) return;
    const seed = input;
    setInput('');
    setSessionId(null);
    setMessages([]);
    setShowReport(false);
    setReportContent('');
    try {
      const res = await fetch(`${BACKEND_URL}/swarm/start`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ seed_text: seed })
      });
      const data = await res.json();
      if (data.session_id) {
        setSessionId(data.session_id);
      }
    } catch (e) {
      console.error("Failed to start swarm:", e);
    }
  };

  const generateReport = async () => {
    if (!sessionId) return;
    try {
      const res = await fetch(`${BACKEND_URL}/swarm/report/${sessionId}`);
      const data = await res.json();
      if (data.markdown) {
        setReportContent(data.markdown);
        setShowReport(true);
      }
    } catch (e) {
      console.error("Failed to fetch report:", e);
    }
  };


  const handleZoom = (type: 'in' | 'out' | 'reset') => {
    if (!svgRef.current || !zoomRef.current) return;
    const svg = d3.select(svgRef.current);
    if (type === 'reset') {
      svg.transition().duration(750).call(zoomRef.current.transform, d3.zoomIdentity);
    } else {
      svg.transition().duration(300).call(zoomRef.current.scaleBy, type === 'in' ? 1.3 : 0.7);
    }
  };

  if (view === 'landing') {
    return (
      <div className="min-h-screen bg-white text-slate-900 flex flex-col overflow-y-auto">
        <header className="px-12 py-8 flex justify-between items-center max-w-7xl mx-auto w-full">
           <div className="flex items-center space-x-3">
              <BrainCircuit className="w-8 h-8 text-[#0d6efd]" />
              <span className="font-bold text-xl tracking-tight uppercase">Sovereign</span>
           </div>
           <nav className="flex items-center space-x-8 text-sm font-semibold text-slate-500">
              <a href="#" className="hover:text-slate-900 transition-colors">Technology</a>
              <a href="#" className="hover:text-slate-900 transition-colors">Repository</a>
              <button onClick={() => setView('grid')} className="bg-[#0d6efd] text-white px-6 py-2 rounded-lg font-bold transition-all hover:shadow-lg active:scale-95">Launch Swarm</button>
           </nav>
        </header>
        <main className="flex-1 flex flex-col items-center justify-center text-center px-6 py-20">
            <h1 className="text-6xl md:text-8xl font-black mb-8 tracking-tight">The first autonomous <br /><span className="text-[#0d6efd]">Local Neural Swarm.</span></h1>
            <p className="max-w-2xl text-xl text-slate-500 mb-12">Experience silicon-grade autonomy with our Clifford-initialized multi-agent engine.</p>
            <button onClick={() => setView('grid')} className="px-10 py-5 bg-[#0d6efd] text-white rounded-xl font-bold flex items-center space-x-3 transition-all hover:scale-105 shadow-2xl">
               <span>Start Live Session</span>
               <ArrowRight className="w-5 h-5" />
            </button>
        </main>
      </div>
    );
  }

  return (
    <div className="agent-canvas h-screen relative overflow-hidden bg-[#fcfcfc]">
      {/* 1. TOP NAV */}
      <div className="h-12 flex items-center justify-between px-10 z-50 pointer-events-none absolute w-full">
         <div className="flex items-center space-x-2 pointer-events-auto">
            <div className="w-1.5 h-1.5 rounded-full bg-emerald-500 animate-pulse" />
            <span className="text-[9px] font-black text-slate-400 uppercase tracking-[0.2em]">Sovereign_Manifold_v1.5</span>
         </div>
         <button 
           onClick={() => setView('landing')}
           className="pointer-events-auto text-[9px] font-bold text-slate-300 hover:text-red-400 uppercase tracking-widest"
         >Terminate Session</button>
      </div>

      {/* 2. D3 MANIFOLD */}
      <svg ref={svgRef} className="w-full h-full z-10 cursor-grab active:cursor-grabbing" viewBox="0 0 1000 1000" />

      {/* 3. FLOATING OVERLAYS */}
      
      {/* Global Command Bar */}
      <div className="absolute top-20 left-1/2 -translate-x-1/2 w-full max-w-lg z-40 transition-all duration-500" style={{ transform: showInspector ? 'translateX(-80%)' : 'translateX(-50%)' }}>
         <form onSubmit={triggerSwarm} className="glass-card flex items-center p-2 rounded-2xl border-white/80 shadow-2xl">
            <div className="w-8 h-8 rounded-lg bg-slate-900 flex items-center justify-center ml-1">
               <ArrowRight className="w-3.5 h-3.5 text-white" />
            </div>
            <input 
               value={input}
               onChange={(e) => setInput(e.target.value)}
               placeholder="Insert Seed Reality..."
               className="flex-1 bg-transparent border-none outline-none px-4 text-[11px] font-bold text-slate-800 placeholder:text-slate-300"
            />
            <button type="submit" disabled={!isLive} className="bg-slate-900 text-white px-6 py-2 rounded-xl text-[10px] font-black uppercase transition-all disabled:opacity-20 active:scale-95">Engage</button>
         </form>
      </div>

      {/* Zoom Toolbox */}
      <div className="absolute top-20 right-10 flex flex-col gap-2 z-40">
         <button onClick={() => handleZoom('in')} className="w-8 h-8 glass-card rounded-lg flex items-center justify-center text-slate-500 hover:text-blue-500 transition-colors"><Plus className="w-4 h-4" /></button>
         <button onClick={() => handleZoom('out')} className="w-8 h-8 glass-card rounded-lg flex items-center justify-center text-slate-500 hover:text-blue-500 transition-colors"><Minus className="w-4 h-4" /></button>
         <button onClick={() => handleZoom('reset')} className="w-8 h-8 glass-card rounded-lg flex items-center justify-center text-slate-500 hover:text-blue-500 transition-colors"><Maximize2 className="w-4 h-4" /></button>
         {sessionId && (
           <button onClick={generateReport} className="mt-4 px-4 py-3 glass-card rounded-xl text-[10px] font-black uppercase tracking-widest text-[#0d6efd] hover:bg-[#0d6efd] hover:text-white transition-all shadow-lg border border-[#0d6efd]/30 whitespace-nowrap">Generate Report</button>
         )}
      </div>


      {/* Entity Legend */}
      <div className="absolute bottom-10 left-10 p-5 glass-card rounded-2xl z-40">
         <h4 className="text-[9px] font-black text-slate-400 uppercase tracking-[0.2em] mb-4">Entity Map</h4>
         <div className="space-y-2.5">
            {[{ l: 'University', c: '#f97316' }, { l: 'Student', c: '#ef4444' }, { l: 'NGO', c: '#3b82f6' }, { l: 'NGO-Meta', c: '#8b5cf6' }].map((item) => (
               <div key={item.l} className="flex items-center gap-3">
                  <div className="w-2.5 h-2.5 rounded-full" style={{ backgroundColor: item.c }} />
                  <span className="text-[9px] font-bold text-slate-700 uppercase">{item.l}</span>
               </div>
            ))}
         </div>
      </div>

      {/* Interaction Feed */}
      {!showInspector && (
        <div className="absolute bottom-10 right-10 w-80 max-h-64 glass-card rounded-2xl overflow-hidden flex flex-col z-40 animate-in fade-in slide-in-from-right-5">
           <div className="p-3 border-b border-white/20 bg-white/40 flex items-center justify-between">
              <span className="text-[9px] font-black uppercase tracking-widest text-slate-500">Interaction_Feed</span>
           </div>
           <div className="flex-1 overflow-y-auto p-4 space-y-3 scrollbar-none" ref={scrollRef}>
              {messages.slice(-5).map((m) => (
                 <div key={m.id} className="animate-in fade-in slide-in-from-bottom-2">
                    <p className="text-[8px] font-black text-blue-500 uppercase tracking-tighter mb-0.5">{m.sender}</p>
                    <p className="text-[10px] text-slate-600 leading-tight font-medium">{m.content}</p>
                 </div>
              ))}
              {messages.length === 0 && <p className="text-[9px] text-slate-300 font-bold uppercase italic text-center p-4">Standby for pulses...</p>}
           </div>
        </div>
      )}

      {/* --- NEURAL INSPECTOR (SIDEBAR) --- */}
      <div className={`absolute top-0 right-0 h-full w-[400px] glass-card border-l border-white/40 z-50 transition-transform duration-500 ease-in-out flex flex-col shadow-[-20px_0_40px_rgba(0,0,0,0.05)] ${showInspector ? 'translate-x-0' : 'translate-x-full'}`}>
         {selectedNode && (
            <>
               <div className="p-8 border-b border-white/20">
                  <div className="flex justify-between items-start mb-6">
                     <div>
                        <div className="flex items-center gap-2 mb-1">
                           <div className="w-2 h-2 rounded-full" style={{ backgroundColor: selectedNode.color || '#3b82f6' }} />
                           <span className="text-[10px] font-black text-slate-400 uppercase tracking-widest">Entity_Inspector</span>
                        </div>
                        <h2 className="text-2xl font-black text-slate-800 tracking-tighter">{selectedNode.label}</h2>
                     </div>
                     <button onClick={() => setShowInspector(false)} className="w-8 h-8 rounded-lg hover:bg-slate-100 flex items-center justify-center transition-colors">
                        <X className="w-5 h-5 text-slate-400" />
                     </button>
                  </div>

                  <div className="grid grid-cols-2 gap-4">
                     <div className="p-3 bg-slate-50/50 rounded-xl border border-slate-100">
                        <p className="text-[8px] font-black text-slate-400 uppercase mb-1">UUID</p>
                        <p className="text-[11px] font-mono font-bold text-slate-700">{selectedNode.uuid}</p>
                     </div>
                     <div className="p-3 bg-slate-50/50 rounded-xl border border-slate-100">
                        <p className="text-[8px] font-black text-slate-400 uppercase mb-1">Status</p>
                        <p className="text-[11px] font-bold text-emerald-600 uppercase flex items-center gap-1">
                           <Activity className="w-3 h-3" /> Synchronized
                        </p>
                     </div>
                  </div>
               </div>

               <div className="flex-1 overflow-y-auto p-8 space-y-8 scrollbar-none">
                  {/* Detailed Knowledge stream */}
                  <div>
                     <h4 className="text-[10px] font-black text-slate-400 uppercase tracking-widest mb-4 flex items-center gap-2">
                        <MessageSquare className="w-3 h-3" /> Agent_Thinking_Stream
                     </h4>
                     <div className="space-y-4">
                        {messages.filter(m => m.sender.includes(selectedNode.id.toUpperCase())).slice(-10).reverse().map((m) => (
                           <div key={m.id} className="p-4 bg-white/60 rounded-2xl border border-white shadow-sm">
                              <div className="flex justify-between items-center mb-2">
                                 <span className="text-[8px] font-black text-blue-500 uppercase">{m.timestamp}</span>
                                 <div className="flex gap-1">
                                    <div className="w-1 h-1 rounded-full bg-slate-200" />
                                    <div className="w-1 h-1 rounded-full bg-slate-200" />
                                 </div>
                              </div>
                              <p className="text-[11px] text-slate-600 leading-relaxed font-medium"><Typewriter text={m.content} /></p>
                           </div>
                        ))}
                        {messages.filter(m => m.sender.includes(selectedNode.id.toUpperCase())).length === 0 && (
                           <p className="text-[10px] text-slate-300 font-bold uppercase italic py-10 text-center">No active pulses detected.</p>
                        )}
                     </div>
                  </div>

                  {/* Relational Facts */}
                  <div>
                     <h4 className="text-[10px] font-black text-slate-400 uppercase tracking-widest mb-4 flex items-center gap-2">
                        <Database className="w-3 h-3" /> Relational_State
                     </h4>
                     <div className="space-y-3">
                        {selectedNode.type === 'agent' && [
                           { label: 'Link Fidelity', value: '0.982' },
                           { label: 'Cluster Rank', value: '#1' },
                           { label: 'Latency', value: '14ms' }
                        ].map(stat => (
                           <div key={stat.label} className="flex justify-between items-center border-b border-slate-100 pb-2">
                              <span className="text-[10px] font-bold text-slate-500 uppercase">{stat.label}</span>
                              <span className="text-[10px] font-mono font-black text-slate-800">{stat.value}</span>
                           </div>
                        ))}
                     </div>
                  </div>
               </div>

               <div className="p-8 bg-slate-50/50 border-t border-white/20">
                  <button className="w-full py-4 bg-slate-900 text-white rounded-2xl text-[10px] font-black uppercase tracking-widest hover:bg-black transition-all active:scale-95 flex items-center justify-center gap-2">
                     <Clock className="w-3.5 h-3.5" /> Replay Agent History
                  </button>
               </div>
            </>
         )}
      </div>

      {/* --- REPORT MODAL --- */}
      {showReport && (
        <div className="absolute inset-0 bg-slate-900/80 backdrop-blur-sm z-[100] flex items-center justify-center p-10 animate-in fade-in">
           <div className="bg-white rounded-3xl w-full max-w-4xl h-full max-h-[85vh] flex flex-col shadow-2xl overflow-hidden border border-white/20">
              <div className="p-6 border-b border-slate-100 flex items-center justify-between bg-slate-50/50">
                 <div className="flex items-center gap-3">
                    <div className="w-10 h-10 rounded-xl bg-blue-100 flex items-center justify-center">
                       <Database className="w-5 h-5 text-blue-600" />
                    </div>
                    <div>
                       <h3 className="text-xl font-black text-slate-800 tracking-tight">Sovereign Swarm Report</h3>
                       <p className="text-[10px] font-bold uppercase tracking-widest text-slate-400">UUID: {sessionId}</p>
                    </div>
                 </div>
                 <button onClick={() => setShowReport(false)} className="w-10 h-10 rounded-xl hover:bg-slate-200 flex items-center justify-center transition-colors">
                    <X className="w-5 h-5 text-slate-500" />
                 </button>
              </div>
              <div className="flex-1 overflow-y-auto p-10 bg-[#fcfcfc] scrollbar-none">
                 <div className="prose prose-sm prose-slate max-w-none">
                    <pre className="font-mono text-xs whitespace-pre-wrap text-slate-700 bg-transparent p-0 m-0 border-none leading-relaxed">
                       {reportContent}
                    </pre>
                 </div>
              </div>
           </div>
        </div>
      )}

      {/* --- NODE HOVER TOOLTIP --- */}
      {hoveredNode && !showInspector && (
        <div 
          className="fixed z-[150] pointer-events-none p-3 glass-card rounded-xl border border-white/50 shadow-xl"
          style={{ left: mousePos.x + 15, top: mousePos.y + 15 }}
        >
           <div className="flex items-center gap-2 mb-1">
              <div className="w-2 h-2 rounded-full" style={{ backgroundColor: hoveredNode.color || '#3b82f6' }} />
              <span className="text-[10px] font-black text-slate-800 uppercase">{hoveredNode.label}</span>
           </div>
           <p className="text-[8px] font-mono text-slate-400">{hoveredNode.uuid}</p>
        </div>
      )}
    </div>
  );
}
