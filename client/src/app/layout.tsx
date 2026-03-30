import type { Metadata } from "next";
import "./globals.css";
import { Home, Share2, Activity, Settings, Cpu, BrainCircuit } from 'lucide-react';

export const metadata: Metadata = {
  title: "Sovereign | Titan Workbench",
  description: "Professional Neural Command Center for the 1.5M Param Titan Swarm.",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en">
      <body className="antialiased font-sans">
        {/* MIRO-STYLE SIDEBAR */}
        <aside className="workbench-sidebar flex flex-col items-center py-8 space-y-8">
            <div className="w-10 h-10 rounded-xl bg-blue-600 flex items-center justify-center shadow-lg shadow-blue-500/20 mb-4">
                <BrainCircuit className="w-6 h-6 text-white" />
            </div>
            
            <nav className="flex-1 w-full px-4 space-y-2">
                <div className="nav-item active">
                    <Home className="w-5 h-5 transition-transform group-hover:scale-110" />
                    <span>Workbench</span>
                </div>
                <div className="nav-item inactive group">
                    <Share2 className="w-5 h-5 transition-transform group-hover:scale-110" />
                    <span>Swarm Hub</span>
                </div>
                <div className="nav-item inactive group">
                    <Activity className="w-5 h-5 transition-transform group-hover:scale-110" />
                    <span>Telemetry</span>
                </div>
            </nav>

            <div className="px-4 w-full">
                <div className="nav-item inactive group">
                    <Settings className="w-5 h-5 transition-transform group-hover:scale-110" />
                    <span>Settings</span>
                </div>
            </div>
        </aside>

        {/* MAIN WORKSPACE */}
        <main className="workbench-main">
            {children}
        </main>
      </body>
    </html>
  );
}
