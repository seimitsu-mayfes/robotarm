import { NextRequest, NextResponse } from 'next/server';

export async function POST(req: NextRequest) {
  try {
    const { query } = await req.json();
    const apiKey = process.env.DIFY_ROBOTCHAT_API_KEY;
    if (!apiKey) {
      console.error("APIキーが設定されていません");
      return NextResponse.json({ error: "APIキー未設定" }, { status: 500 });
    }
    if (!query) {
      console.error("queryが空です");
      return NextResponse.json({ error: "query未設定" }, { status: 400 });
    }

    const difyRes = await fetch('https://api.dify.ai/v1/chat-messages', {
      method: 'POST',
      headers: {
        'Authorization': `Bearer ${apiKey}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        inputs: {},
        query,
        user: "robotarm-frontend",
        response_mode: 'blocking',
        response_format: 'json',
      }),
    });

    const text = await difyRes.text();
    console.log("Dify API response:", text);

    if (!difyRes.ok) {
      return NextResponse.json({ error: 'Dify API error', detail: text }, { status: 500 });
    }

    const data = JSON.parse(text);

    let answerObj = data.answer;
    if (typeof answerObj === "string") {
      try {
        answerObj = JSON.parse(answerObj);
      } catch {
        // パース失敗時はそのまま
      }
    }

    // chat/act形式で返ってくる場合
    const responseData = answerObj ?? data;

    const actionId = String(Date.now()) + Math.random().toString(36).slice(2, 8);
    if (responseData.act) {
      // BLEアクションはawaitせず非同期で実行
      fetch("http://localhost:8000/action", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ action: responseData.act, action_id: actionId }),
      });
    }
    return NextResponse.json({ ...responseData, action_id: actionId });
  } catch (e) {
    console.error("APIルートで例外:", e);
    return NextResponse.json({ error: String(e) }, { status: 500 });
  }
} 