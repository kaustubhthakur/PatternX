"use client";

import { FormEvent, Suspense, useState } from "react";
import { useRouter, useSearchParams } from "next/navigation";
import { authApi, ApiError } from "@/lib/api";
import { useAuth } from "@/context/AuthContext";

function VerifyOtpForm() {
  const router = useRouter();
  const searchParams = useSearchParams();
  const { setUser } = useAuth();
  const userId = searchParams.get("userId") || "";

  const [otp, setOtp] = useState("");
  const [error, setError] = useState<string | null>(null);
  const [submitting, setSubmitting] = useState(false);

  const handleSubmit = async (e: FormEvent) => {
    e.preventDefault();
    setError(null);

    if (!userId) {
      setError("Missing user reference — please log in again.");
      return;
    }

    setSubmitting(true);
    try {
      const res = await authApi.verifyOtp({ userId, otp });
      // Cookie is already set by the server (httpOnly). We just keep a
      // local copy of the user for the UI.
      setUser(res.user);
      router.push("/");
    } catch (err) {
      setError(err instanceof ApiError ? err.message : "Verification failed");
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <div className="auth-shell">
      <div className="auth-card">
        <h1>Enter the code</h1>
        <p style={{ color: "#555", fontSize: "0.9rem", marginTop: -10 }}>
          We emailed a 6-digit code — it expires in 5 minutes.
        </p>
        {error && <div className="error-text">{error}</div>}
        <form onSubmit={handleSubmit}>
          <div className="field">
            <label htmlFor="otp">One-time code</label>
            <input
              id="otp"
              value={otp}
              onChange={(e) => setOtp(e.target.value)}
              maxLength={6}
              inputMode="numeric"
              autoFocus
              required
            />
          </div>
          <button className="btn" type="submit" disabled={submitting}>
            {submitting ? "Verifying..." : "Verify"}
          </button>
        </form>
      </div>
    </div>
  );
}

export default function VerifyOtpPage() {
  return (
    <Suspense fallback={null}>
      <VerifyOtpForm />
    </Suspense>
  );
}