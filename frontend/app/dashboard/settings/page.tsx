export default function SettingsPage() {
  return (
    <div className="page">
      <div className="page-header">
        <h1>Settings</h1>
        <p className="page-subtitle">Manage your account preferences.</p>
      </div>
      <div className="panel">
        <div className="field">
          <label htmlFor="displayName">Display name</label>
          <input id="displayName" placeholder="Your name" />
        </div>
        <div className="field">
          <label htmlFor="email">Email notifications</label>
          <input id="email" type="email" placeholder="you@example.com" />
        </div>
        <button className="btn" style={{ width: "auto", padding: "10px 20px" }}>
          Save changes
        </button>
      </div>
    </div>
  );
}