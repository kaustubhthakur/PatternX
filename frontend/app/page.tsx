"use client";

import Link from "next/link";
import { useAuth } from "@/context/AuthContext";

export default function HomePage() {
  const { user, loading } = useAuth();

  if (loading) return null;

  if (!user) {
    return (
      <div className="auth-shell">
        <div className="auth-card">
          <h1>You&apos;re not signed in</h1>
          <Link href="/login">
            <button className="btn">Go to login</button>
          </Link>
          <p className="helper-text">
            No account? <Link href="/register">Register</Link>
          </p>
        </div>
      </div>
    );
  }

  return (
    <div className="page">
      <div className="page-header">
        <h1>Welcome back, Lord {user.username}</h1>
        
      </div>
    </div>
  );
}