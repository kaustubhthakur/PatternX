"use client";

import { usePathname } from "next/navigation";
import Navbar from "./Navbar";
import Footer from "./Footer";
import { useAuth } from "@/context/AuthContext";

const AUTH_ROUTES = ["/login", "/register", "/verify-otp"];

export default function AppChrome({
  children,
}: {
  children: React.ReactNode;
}) {
  const pathname = usePathname();
  const { user, loading } = useAuth();

  const isAuthRoute = AUTH_ROUTES.some((route) => pathname.startsWith(route));

  // On login/register/verify-otp, or before we know the auth state, or
  // when logged out — render the page as-is, no chrome.
  if (isAuthRoute || loading || !user) {
    return <>{children}</>;
  }

  return (
    <div className="app-shell">
      <Navbar />
      <main className="app-main">{children}</main>
      <Footer />
    </div>
  );
}