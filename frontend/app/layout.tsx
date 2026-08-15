import type { Metadata } from "next";
import { AuthProvider } from "@/context/AuthContext";
import { ThemeProvider } from "@/context/ThemeContext";
import AppChrome from "@/components/AppChrome";
import "./globals.css";

export const metadata: Metadata = {
  title: "Auth Demo",
  description: "Next.js frontend for the Express auth API",
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en" suppressHydrationWarning>
      <body>
        <ThemeProvider>
          <AuthProvider>
            <AppChrome>{children}</AppChrome>
          </AuthProvider>
        </ThemeProvider>
      </body>
    </html>
  );
}