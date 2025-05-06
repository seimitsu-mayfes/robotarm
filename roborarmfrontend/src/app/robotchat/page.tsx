"use client";
import { useState } from "react";
import type { RobotArmAction } from "../../lib/robotActions";

export default function RobotChatPage() {
  const [query, setQuery] = useState("");
  const [response, setResponse] = useState<{ chat: string; act: RobotArmAction } | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setLoading(true);
    setError("");
    setResponse(null);
    try {
      const res = await fetch("/api/robotchat", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ query }),
      });
      if (!res.ok) {
        throw new Error("APIエラー");
      }
      const data = await res.json();
      setResponse(data);
    } catch (err: any) {
      setError(err.message || "不明なエラー");
    } finally {
      setLoading(false);
    }
  };

  return (
    <main style={{ maxWidth: 600, margin: "2rem auto", padding: 16 }}>
      <h1>ロボットアーム チャット</h1>
      <form onSubmit={handleSubmit} style={{ marginBottom: 24 }}>
        <input
          type="text"
          value={query}
          onChange={e => setQuery(e.target.value)}
          placeholder="質問や指示を入力..."
          style={{ width: "80%", padding: 8, fontSize: 16 }}
          disabled={loading}
        />
        <button type="submit" disabled={loading || !query} style={{ marginLeft: 8, padding: 8 }}>
          送信
        </button>
      </form>
      {loading && <div>送信中...</div>}
      {error && <div style={{ color: "red" }}>エラー: {error}</div>}
      {response && (
        <div style={{ border: "1px solid #ccc", borderRadius: 8, padding: 16 }}>
          <div><strong>チャット応答:</strong><br />{response.chat}</div>
          <div style={{ marginTop: 12 }}><strong>アクション:</strong><br />{response.act}</div>
        </div>
      )}
    </main>
  );
} 