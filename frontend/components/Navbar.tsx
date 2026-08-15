"use client";

import Link from "next/link";
import { usePathname } from "next/navigation";
import ThemeToggle from "./ThemeToggle";
import { useAuth } from "@/context/AuthContext";

const TABS = [
  { href: "/dashboard", label: "Home" },
  { href: "/dashboard/analytics", label: "Analytics" },
  { href: "/dashboard/reports", label: "Reports" },
  { href: "/dashboard/settings", label: "Settings" },
];

export default function Navbar() {
  const pathname = usePathname();
  const { user, logout } = useAuth();

  return (
    <header className="navbar">
      <div className="navbar-inner">
        <Link href="/dashboard" className="navbar-brand">
          Dashboard
        </Link>

        <nav className="navbar-tabs">
          {TABS.map((tab) => {
            const active =
              tab.href === "/dashboard"
                ? pathname === "/dashboard"
                : pathname.startsWith(tab.href);
            return (
              <Link
                key={tab.href}
                href={tab.href}
                className={`navbar-tab ${active ? "active" : ""}`}
              >
                {tab.label}
              </Link>
            );
          })}
        </nav>

        <div className="navbar-actions">
          <ThemeToggle />
          {user && (
            <div className="navbar-user">
              <span className="navbar-username">{user.username}</span>
              <button className="btn secondary sm" onClick={logout}>
                Log out
              </button>
            </div>
          )}
        </div>
      </div>
    </header>
  );
}