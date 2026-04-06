import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "Swarm-Engine | 1.5M Parameter Local Engine",
  description: "Privacy-focused, 1.5M parameter local inference engine.",
};

export default function RootLayout({
  children,
}: Readonly<{
  children: React.ReactNode;
}>) {
  return (
    <html lang="en" suppressHydrationWarning>
      <body className="antialiased font-sans selection:bg-blue-500/10">
        {children}
      </body>
    </html>
  );
}
