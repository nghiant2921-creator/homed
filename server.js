import express from 'express';

const app = express();
app.use(express.json());

// Bật CORS để cho phép gọi API từ trình duyệt hoặc ứng dụng ngoài Internet
app.use((req, res, next) => {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
    if (req.method === 'OPTIONS') {
        res.header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
        return res.status(200).json({});
    }
    next();
});

// Trang giao diện Web để chat trực tiếp trên trình duyệt
app.get('/', (req, res) => {
    res.send(`<!DOCTYPE html>
<html lang="vi">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Local AI Chatbot</title>
  <style>
    body { font-family: system-ui, -apple-system, sans-serif; background: #0f172a; color: #f8fafc; margin: 0; padding: 30px 20px; display: flex; justify-content: center; }
    .container { width: 100%; max-width: 640px; background: #1e293b; padding: 24px; border-radius: 12px; box-shadow: 0 4px 20px rgba(0,0,0,0.3); }
    h1 { font-size: 1.4rem; margin-top: 0; color: #38bdf8; display: flex; align-items: center; gap: 8px; }
    .status { font-size: 0.85rem; color: #34d399; margin-bottom: 16px; display: inline-block; }
    p { color: #94a3b8; font-size: 0.95rem; margin-bottom: 8px; }
    textarea { width: 100%; height: 90px; background: #0f172a; border: 1px solid #334155; border-radius: 8px; color: #fff; padding: 12px; box-sizing: border-box; font-size: 0.95rem; resize: vertical; }
    textarea:focus { outline: none; border-color: #38bdf8; }
    button { margin-top: 10px; background: #0284c7; color: white; border: none; padding: 10px 22px; border-radius: 6px; font-size: 0.95rem; font-weight: 600; cursor: pointer; transition: 0.2s; }
    button:hover { background: #0369a1; }
    button:disabled { background: #475569; cursor: not-allowed; }
    .chat-box { margin-top: 20px; padding: 16px; background: #0f172a; border-radius: 8px; border: 1px solid #334155; min-height: 80px; white-space: pre-wrap; font-size: 0.95rem; line-height: 1.6; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🤖 Local AI Chatbot</h1>
    <span class="status">● Server Online (Cổng 3000)</span>
    <p>Nhập câu hỏi của bạn:</p>
    <textarea id="prompt" placeholder="Ví dụ: Giải thích cơ chế Event Loop trong Node.js..."></textarea>
    <br>
    <button id="sendBtn" onclick="sendChat()">Gửi câu hỏi</button>
    <h3 style="margin-top:24px; color:#cbd5e1; font-size:1rem;">Phản hồi từ AI:</h3>
    <div id="responseBox" class="chat-box">Câu trả lời sẽ xuất hiện ở đây...</div>
  </div>

  <script>
    async function sendChat() {
      const promptInput = document.getElementById('prompt');
      const sendBtn = document.getElementById('sendBtn');
      const responseBox = document.getElementById('responseBox');
      const prompt = promptInput.value.trim();

      if (!prompt) return;

      sendBtn.disabled = true;
      sendBtn.innerText = 'Đang suy nghĩ...';
      responseBox.innerText = '⏳ AI đang xử lý câu trả lời...';

      try {
        const res = await fetch('/api/chat', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ prompt })
        });
        const data = await res.json();
        if (data.reply) {
          responseBox.innerText = data.reply;
        } else if (data.error) {
          responseBox.innerText = '❌ ' + data.error;
        }
      } catch (err) {
        responseBox.innerText = '❌ Lỗi kết nối: ' + err.message;
      } finally {
        sendBtn.disabled = false;
        sendBtn.innerText = 'Gửi câu hỏi';
      }
    }
  </script>
</body>
</html>`);
});

// Hướng dẫn nếu vô tình mở /api/chat bằng trình duyệt (GET)
app.get('/api/chat', (req, res) => {
    res.json({
        message: 'Endpoint này yêu cầu phương thức POST thay vì GET.',
        huong_dan: 'Hãy truy cập http://localhost:3000/ trên trình duyệt để sử dụng giao diện chat, hoặc gửi POST request kèm body: { "prompt": "câu hỏi" }.'
    });
});

// API nhận câu hỏi qua method POST
app.post('/api/chat', async (req, res) => {
    const { prompt } = req.body;

    if (!prompt) {
        return res.status(400).json({ error: 'Vui lòng cung cấp prompt' });
    }

    try {
        console.log(`📥 Nhận câu hỏi: "${prompt}"`);

        // Gọi tới Ollama đang chạy trên máy (cổng 11434)
        const response = await fetch('http://127.0.0.1:11434/api/generate', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                model: 'gemma2:2b', // hoặc tên model trong máy bạn
                prompt: prompt,
                stream: false
            })
        });

        const data = await response.json();
        console.log(`🤖 Đã tạo câu trả lời thành công!`);

        res.json({ reply: data.response });
    } catch (error) {
        console.error('Lỗi Ollama:', error);
        res.status(500).json({ error: 'Không thể kết nối đến Ollama. Hãy đảm bảo Ollama đang chạy!' });
    }
});

const PORT = 3000;
app.listen(PORT, () => {
    console.log(`🚀 API Server đang chạy tại: http://localhost:${PORT}`);
    console.log(`👉 Mở trình duyệt tại: http://localhost:${PORT}`);
});
