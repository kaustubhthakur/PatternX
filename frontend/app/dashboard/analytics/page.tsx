export default function AnalyticsPage() {
  return (
    <div className="page">
      <div className="page-header">
        <h1>Analytics</h1>
        <p className="page-subtitle">Traffic and engagement over time.</p>
      </div>
      <div className="panel">
        <h2>Overview</h2>
        <p style={{ color: "var(--text-muted)" }}>
          Plug your charting library of choice in here (Recharts, Chart.js,
          etc).
        </p>
      </div>
    </div>
  );
}