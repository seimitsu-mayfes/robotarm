import Link from "next/link";

export default function Home() {
  return (
    <main>
      <h1>ロボットアームフロントエンド</h1>
      <ul>
        <li>
          <Link href="/robotchat">ロボットアーム チャットページへ</Link>
        </li>
      </ul>
      {/* 既存の内容があればここに追加 */}
    </main>
  );
} 