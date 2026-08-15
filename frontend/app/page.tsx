"use client";

import Link from "next/link";
import { useAuth } from "@/context/AuthContext";

export default function HomePage() {
  const { user, logout, loading } = useAuth();

  if (loading) return null;

  return (
    <div className="auth-shell">
      <div className="auth-card">
        {user ? (
          <>
            <h1>Welcome, {user.username}</h1>
            <p style={{ color: "#555", marginBottom: 20 }}>{user.email}</p>
            <button className="btn" onClick={logout}>
              Log out
            </button>
          </>
        ) : (
          <>
            <h1>You&apos;re not signed in</h1>
            <Link href="/login">
              <button className="btn">Go to login</button>
            </Link>
            <p className="helper-text">
              No account? <Link href="/register">Register</Link>
            </p>
          </>
        )}
      </div>
    </div>
  );
}