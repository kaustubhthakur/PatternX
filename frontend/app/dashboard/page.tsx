"use client";

import { useAuth } from "@/context/AuthContext";

const STATS = [
  { label: "Total Users", value: "12,489", change: "+4.3%" },
  { label: "Revenue", value: "$48,920", change: "+8.1%" },
  { label: "Active Sessions", value: "1,204", change: "-1.2%" },
  { label: "Conversion Rate", value: "3.6%", change: "+0.4%" },
];

export default function DashboardHome() {
  const { user } = useAuth();

  return (
    <div className="page">
      <div className="page-header">
        <h1>Welcome back{user ? `, ${user.username}` : ""}</h1>
        <p className="page-subtitle">Here&apos;s what&apos;s happening today.</p>
      </div>

      <div className="stat-grid">
        {STATS.map((stat) => (
          <div key={stat.label} className="stat-card">
            <span className="stat-label">{stat.label}</span>
            <div className="stat-row">
              <span className="stat-value">{stat.value}</span>
              <span
                className={`stat-change ${
                  stat.change.startsWith("-") ? "negative" : "positive"
                }`}
              >
                {stat.change}
              </span>
            </div>
          </div>
        ))}
      </div>

      <div className="panel">
        <h2>Recent Activity</h2>
        <ul className="activity-list">
          <li>
            <span>New user registered</span>
            <span className="activity-time">2m ago</span>
          </li>
          <li>
            <span>Payment received from Acme Corp</span>
            <span className="activity-time">18m ago</span>
          </li>
          <li>
            <span>Server deployment completed</span>
            <span className="activity-time">1h ago</span>
          </li>
        </ul>
      </div>
    </div>
  );
}