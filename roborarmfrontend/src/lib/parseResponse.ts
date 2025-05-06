interface ChatResponse {
  chat: string;
  id: number;
  name: string;
  probability: number;
}

export const parseResponse = (content: string): ChatResponse | null => {
  try {
    // Try to parse the entire content as JSON
    const jsonResponse = JSON.parse(content);
    
    // Validate the structure
    if (
      typeof jsonResponse.chat === 'string' &&
      typeof jsonResponse.id === 'number' &&
      typeof jsonResponse.name === 'string' &&
      typeof jsonResponse.probability === 'number' &&
      jsonResponse.probability >= 0 &&
      jsonResponse.probability <= 1
    ) {
      return jsonResponse;
    }
    
    return null;
  } catch {
    return null;
  }
}; 