"use client";
import React, { useState, useEffect, useRef, useImperativeHandle, forwardRef } from "react";
import type { RobotArmAction } from "../../lib/robotActions";

// 型定義: ブラウザのWeb Speech API用
declare global {
  interface Window {
    webkitSpeechRecognition: any;
    SpeechRecognition: any;
  }
  var webkitSpeechRecognition: any;
  var SpeechRecognition: any;
  interface SpeechRecognitionEvent extends Event {
    results: {
      [index: number]: {
        [index: number]: { transcript: string }
      }
    }
  }
}

// SpeechInputButtonをforwardRefでラップ
const SpeechInputButton = forwardRef(function SpeechInputButton(
  { onResult, disabled }: { onResult: (text: string) => void, disabled?: boolean },
  ref
) {
  const recognitionRef = useRef<any>(null);
  const [listening, setListening] = useState(false);

  const startRecognition = () => {
    if (!('webkitSpeechRecognition' in window || 'SpeechRecognition' in window)) {
      alert('このブラウザは音声認識に対応していません');
      return;
    }
    const SpeechRecognition: any = (window as any).webkitSpeechRecognition || (window as any).SpeechRecognition;
    const recognition = new SpeechRecognition();
    recognition.lang = 'ja-JP';
    recognition.interimResults = false;
    recognition.onstart = () => {
      console.log('[音声認識] 開始');
    };
    recognition.onaudiostart = () => {
      console.log('[音声認識] 音声入力開始');
    };
    recognition.onspeechstart = () => {
      console.log('[音声認識] 発話検出');
    };
    recognition.onspeechend = () => {
      console.log('[音声認識] 発話終了');
    };
    recognition.onresult = (event: any) => {
      const text = event.results[0][0].transcript;
      console.log('[音声認識] 認識結果:', text);
      onResult(text);
      setListening(false);
    };
    recognition.onend = () => {
      console.log('[音声認識] 認識終了');
      setListening(false);
    };
    recognition.onerror = (e: any) => {
      console.log('[音声認識] エラー:', e.error);
      setListening(false);
    };
    recognitionRef.current = recognition;
    recognition.start();
    setListening(true);
    console.log('[音声認識] 認識処理開始');
  };

  const stopRecognition = () => {
    recognitionRef.current?.stop();
    setListening(false);
    console.log('[音声認識] 手動停止');
  };

  useImperativeHandle(ref, () => ({
    startRecognition,
    stopRecognition,
    isListening: () => listening,
  }));

  return (
    <button
      type="button"
      onClick={listening ? stopRecognition : startRecognition}
      disabled={disabled}
      style={{
        marginLeft: 8,
        padding: '0 18px',
        fontSize: 18,
        borderRadius: 8,
        border: '1.5px solid #ccc',
        background: listening ? '#ff9800' : '#f5f5f5',
        color: listening ? '#fff' : '#222',
        cursor: disabled ? 'not-allowed' : 'pointer',
        height: 48
      }}
    >
      {listening ? '認識中…' : '🎤 音声'}
    </button>
  );
});

export default function RobotChatPage() {
  const [query, setQuery] = useState("");
  const [response, setResponse] = useState<{ chat: string; act: RobotArmAction; action_id?: string } | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState("");
  const [actionStatus, setActionStatus] = useState<"pending" | "done" | "unknown">("done");
  const pollingRef = useRef<NodeJS.Timeout | null>(null);
  const autoSendTimerRef = useRef<NodeJS.Timeout | null>(null);
  const speechButtonRef = useRef<any>(null);

  useEffect(() => {
    if (response?.action_id) {
      setActionStatus("pending");
      // ポーリング開始
      pollingRef.current = setInterval(async () => {
        const res = await fetch(`/api/action_status_proxy?action_id=${response.action_id}`);
        const data = await res.json();
        if (data.status === "done") {
          setActionStatus("done");
          if (pollingRef.current) clearInterval(pollingRef.current);
        }
      }, 2000);
      return () => {
        if (pollingRef.current) clearInterval(pollingRef.current);
      };
    } else {
      setActionStatus("done");
    }
  }, [response?.action_id]);

  // actionStatusがdoneになったら入力欄をクリアし、音声認識を自動再開
  /*
  useEffect(() => {
    if (actionStatus === "done") {
      setQuery("");
      // 音声認識が止まっている場合のみ再開
      if (speechButtonRef.current && !speechButtonRef.current.isListening()) {
        speechButtonRef.current.startRecognition();
      }
    }
  }, [actionStatus]);
  */

  useEffect(() => {
    if (response?.chat) {
      const utter = new window.SpeechSynthesisUtterance(response.chat);
      utter.lang = "ja-JP";
      const voices = window.speechSynthesis.getVoices();
      const jaVoice = voices.find(v => v.lang === "ja-JP");
      if (jaVoice) utter.voice = jaVoice;
      window.speechSynthesis.cancel();
      window.speechSynthesis.speak(utter);
    }
  }, [response?.chat]);

  const sendQuery = async (text: string) => {
    setLoading(true);
    setError("");
    setResponse(null);
    setActionStatus("done");
    try {
      const res = await fetch("/api/robotchat", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ query: text }),
      });
      if (!res.ok) throw new Error("APIエラー");
      const data = await res.json();
      setResponse(data);
    } catch (err: any) {
      setError(err.message || "不明なエラー");
    } finally {
      setLoading(false);
    }
  };

  const handleSpeechResult = (text: string) => {
    setQuery(text);
    if (autoSendTimerRef.current) clearTimeout(autoSendTimerRef.current);
    console.log('[音声認識] 自動送信タイマー開始: 0.5秒');
    autoSendTimerRef.current = setTimeout(() => {
      if (text.trim() && !loading && actionStatus !== "pending") {
        console.log('[音声認識] 0.5秒経過で自動送信:', text);
        sendQuery(text);
      }
    }, 500);
  };

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (!query.trim() || loading || actionStatus === "pending") return;
    sendQuery(query);
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === "Enter" && !loading && actionStatus !== "pending" && query.trim()) {
      e.preventDefault();
      sendQuery(query);
    }
  };

  return (
    <div style={{ minHeight: "100vh", display: "flex", flexDirection: "column", justifyContent: "space-between" }}>
      <div style={{ position: "fixed", top: 12, left: 18, fontSize: 13, color: "#888", zIndex: 1000, letterSpacing: 1, userSelect: "none" }}>
        2025年五月祭 精密工学科 ロボットアーム企画
      </div>
      <div style={{ flex: 1, display: "flex", flexDirection: "column", alignItems: "center", justifyContent: "center" }}>
        {error && <div style={{ color: "red", marginBottom: 16 }}>エラー: {error}</div>}
        {loading ? (
          <div style={{ height: 80, display: "flex", alignItems: "center", justifyContent: "center" }}>
            <span style={{ display: "inline-block", width: 80, textAlign: "center" }}>
              <span className="dot" style={{
                display: "inline-block",
                width: 18,
                height: 18,
                margin: "0 6px",
                borderRadius: "50%",
                background: "#ff9800",
                opacity: 0.5,
                animation: "dotFade 1.2s infinite",
                animationDelay: "0s"
              }}></span>
              <span className="dot" style={{
                display: "inline-block",
                width: 18,
                height: 18,
                margin: "0 6px",
                borderRadius: "50%",
                background: "#ff9800",
                opacity: 0.5,
                animation: "dotFade 1.2s infinite",
                animationDelay: "0.2s"
              }}></span>
              <span className="dot" style={{
                display: "inline-block",
                width: 18,
                height: 18,
                margin: "0 6px",
                borderRadius: "50%",
                background: "#ff9800",
                opacity: 0.5,
                animation: "dotFade 1.2s infinite",
                animationDelay: "0.4s"
              }}></span>
              <style>{`
                @keyframes dotFade {
                  0%, 80%, 100% { opacity: 0.2; transform: scale(0.8); }
                  40% { opacity: 1; transform: scale(1.2); }
                }
              `}</style>
            </span>
          </div>
        ) : response && (
          <>
            <div
              style={{
                color: actionStatus === "pending" ? "#ff9800" : "#222",
                fontSize: actionStatus === "pending" ? "3.5rem" : "3rem",
                fontWeight: 700,
                textAlign: "center",
                marginBottom: 12,
                maxWidth: 800,
                wordBreak: "break-word",
                lineHeight: 1.25,
                transition: "color 0.3s, font-size 0.3s",
                animation: actionStatus === "pending" ? "bounce 0.7s infinite alternate" : "none"
              }}
            >
              {response.chat}
            </div>
            <style>{`
              @keyframes bounce {
                0% { transform: translateY(0); }
                100% { transform: translateY(-18px); }
              }
            `}</style>
            {response.act && (
              <div style={{ fontSize: "1.2rem", color: "#222", textAlign: "center", marginBottom: 8, fontWeight: 500 }}>
                {response.act}
              </div>
            )}
          </>
        )}
      </div>
      <form
        onSubmit={handleSubmit}
        style={{
          position: "sticky",
          bottom: 0,
          width: "100%",
          background: "#fff",
          padding: "16px 0 24px 0",
          display: "flex",
          justifyContent: "center",
          borderTop: "1px solid #eee"
        }}
        autoComplete="off"
      >
        <input
          type="text"
          value={query}
          onChange={e => setQuery(e.target.value)}
          onKeyDown={handleKeyDown}
          placeholder="質問や指示を入力..."
          style={{ width: "60%", padding: 16, fontSize: 22, borderRadius: 12, border: "1.5px solid #ccc" }}
          disabled={loading || actionStatus === "pending"}
        />
        <SpeechInputButton
          ref={speechButtonRef}
          onResult={handleSpeechResult}
          disabled={loading || actionStatus === "pending"}
        />
      </form>
    </div>
  );
} 