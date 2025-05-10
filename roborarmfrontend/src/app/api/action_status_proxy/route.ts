import { NextRequest, NextResponse } from 'next/server';

export async function GET(req: NextRequest) {
  const { searchParams } = new URL(req.url);
  const action_id = searchParams.get('action_id');
  if (!action_id) {
    return NextResponse.json({ status: 'unknown' }, { status: 400 });
  }
  const res = await fetch(`http://localhost:8000/action_status?action_id=${encodeURIComponent(action_id)}`);
  const data = await res.json();
  return NextResponse.json(data);
} 