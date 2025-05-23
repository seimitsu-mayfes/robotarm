"use client";
import React, { useState } from "react";

export default function VoiceTestPage() {
  const [text, setText] = useState("");
  const [speaking, setSpeaking] = useState(false);

  const speak = () => {
    if (!text.trim()) return;
    const utter = new window.SpeechSynthesisUtterance(text);
    utter.lang = "ja-JP";
    const voices = window.speechSynthesis.getVoices();
    const jaVoice = voices.find(v => v.lang === "ja-JP");
    if (jaVoice) utter.voice = jaVoice;
    utter.onstart = () => setSpeaking(true);
    utter.onend = () => setSpeaking(false);
    window.speechSynthesis.cancel();
    window.speechSynthesis.speak(utter);
  };

  return (
    <div style={{ minHeight: "100vh", display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center" }}>
      <h2>音声テストページ</h2>
      <input
        type="text"
        value={text}
        onChange={e => setText(e.target.value)}
        placeholder="ここにテキストを入力"
        style={{ width: 320, fontSize: 20, padding: 12, borderRadius: 8, border: "1.5px solid #ccc", marginBottom: 16 }}
        disabled={speaking}
      />
      <button
        onClick={speak}
        disabled={speaking || !text.trim()}
        style={{ fontSize: 20, padding: "8px 32px", borderRadius: 8, background: speaking ? "#ff9800" : "#2196f3", color: "#fff", border: "none", cursor: speaking ? "not-allowed" : "pointer" }}
      >
        {speaking ? "再生中..." : "▶ 読み上げ"}
      </button>
    </div>
  );
} 