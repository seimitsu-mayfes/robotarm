/**
 * Dify APIとの通信を管理するモジュール
 * チャットとワークフローの両方のAPIエンドポイントを処理
 */
import axios from 'axios';

// APIエンドポイントの設定
const API_BASE_URL = '/api';

/**
 * チャットメッセージの型定義
 * ユーザーとアシスタントの対話を表現
 */
export interface ChatMessage {
  role: 'user' | 'assistant';
  content: string;
}

/**
 * Server-Sent Events (SSE) のデータ型定義
 * Difyからのストリーミングレスポンスの各イベントタイプを定義
 */
interface StreamData {
  event: 'message' | 'agent_message' | 'tts_message' | 'tts_message_end' | 'agent_thought' | 'message_file' | 'message_end' | 'message_replace' | 'error' | 'ping';
  task_id?: string;
  message_id?: string;
  conversation_id?: string;
  answer?: string;
  audio?: string;
  created_at?: number;
  metadata?: {
    usage?: {
      prompt_tokens: number;
      completion_tokens: number;
      total_tokens: number;
    };
  };
  status?: number;
  code?: string;
  message?: string;
}

/**
 * ストリーム制御インターフェース
 * SSEストリームの制御と状態管理を行う
 */
interface StreamController {
  stream: Promise<void>;
  abort: () => void;
  onMessage: ((data: StreamData) => void) | null;
  onDone: (() => void) | null;
  onError: ((error: Error) => void) | null;
}

/**
 * SSEストリームを作成し、Dify APIとの通信を確立する関数
 * @param message ユーザーからのメッセージ
 * @param conversationId 会話を識別するID（オプション）
 * @returns ストリーム制御オブジェクト
 */
export const createEventSource = (message: string, conversationId?: string, endpoint: string = 'chat'): StreamController => {
  console.log(`[${new Date().toISOString()}] Starting SSE connection:`, {
    endpoint,
    conversationId,
    messageLength: message.length
  });
  
  const controller = new AbortController();
  
  const streamController: StreamController = {
    stream: new Promise<void>((resolve, reject) => {
      fetch(`${API_BASE_URL}/${endpoint}`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
          'Accept': 'text/event-stream'
        },
        body: JSON.stringify({
          message,
          conversationId
        }),
        signal: controller.signal
      }).then(async response => {
        console.log(`[${new Date().toISOString()}] SSE connection established:`, {
          status: response.status,
          headers: Object.fromEntries(response.headers.entries())
        });

        if (!response.ok) {
          throw new Error(`HTTP error! status: ${response.status}`);
        }
        
        const reader = response.body?.getReader();
        if (!reader) {
          throw new Error('Response body is null');
        }

        const decoder = new TextDecoder();
        try {
          while (true) {
            const { value, done } = await reader.read();
            if (done) {
              streamController.onDone?.();
              resolve();
              break;
            }

            const text = decoder.decode(value, { stream: true });
            const chunks = text.split('\n');
            
            console.log(`[${new Date().toISOString()}] Received data chunk:`, {
              chunkCount: chunks.length,
              totalBytes: value.length
            });
            
            for (const chunk of chunks) {
              if (!chunk.trim() || !chunk.startsWith('data: ')) continue;
              
              try {
                const data = JSON.parse(chunk.slice(6)) as StreamData;
                console.log(`[${new Date().toISOString()}] Processed event:`, {
                  event: data.event,
                  hasAnswer: !!data.answer,
                  messageId: data.message_id
                });
                
                if (data.event === 'message' && data.answer) {
                  streamController.onMessage?.(data);
                } else if (data.event === 'message_end') {
                  console.log(`[${new Date().toISOString()}] Stream completed`, {
                    totalTokens: data.metadata?.usage?.total_tokens
                  });
                  streamController.onDone?.();
                  resolve();
                  return;
                } else if (data.event === 'error') {
                  const error = new Error(data.message || 'Unknown error');
                  streamController.onError?.(error);
                  reject(error);
                  return;
                }
              } catch (e) {
                console.error(`[${new Date().toISOString()}] Error parsing JSON:`, {
                  error: e,
                  rawChunk: chunk
                });
              }
            }
          }
        } catch (error) {
          console.error(`[${new Date().toISOString()}] Stream error:`, {
            error,
            endpoint,
            conversationId
          });
          const errorObj = error instanceof Error ? error : new Error('Stream error');
          streamController.onError?.(errorObj);
          reject(errorObj);
        } finally {
          reader.releaseLock();
        }
      }).catch(error => {
        console.error(`[${new Date().toISOString()}] Connection error:`, {
          error,
          endpoint,
          conversationId
        });
        const errorObj = error instanceof Error ? error : new Error('Failed to connect');
        streamController.onError?.(errorObj);
        reject(errorObj);
      });
    }),
    abort: () => controller.abort(),
    onMessage: null,
    onDone: null,
    onError: null
  };

  return streamController;
};

/**
 * Difyのワークフローを実行する関数
 * ユーザーの入力に基づいて料理提案を生成
 * @param query ユーザーからの料理に関する要望
 * @returns ワークフロー実行結果
 */
export const executeWorkflow = async (query: string) => {
  try {
    // ワークフローAPIにリクエストを送信
    const response = await axios.post(`${API_BASE_URL}/workflow`, 
      { query },
      {
        headers: {
          'Content-Type': 'application/json',
          'Accept': '*/*'
        }
      }
    );
    return response.data;
  } catch (error) {
    console.error('Workflow API Error:', error);
    throw error;
  }
}; 