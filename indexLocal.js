// Gọi Local AI (Ollama) chạy offline trên máy tính
async function askLocalAI(prompt) {
    try {
        console.log('🤖 Đang gửi yêu cầu tới Local AI (Ollama)...');

        const response = await fetch('http://localhost:11434/api/generate', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                model: 'gemma2:2b', // Có thể đổi sang 'llama3.2:1b' hoặc 'qwen2.5:1.5b'
                prompt: prompt,
                stream: false, // Nhận trọn vẹn câu trả lời thay vì từng từ
            }),
        });

        if (!response.ok) {
            throw new Error(`Lỗi Ollama (HTTP ${response.status}). Hãy kiểm tra xem Ollama có đang chạy không.`);
        }

        const data = await response.json();
        console.log('\n--- Phản hồi từ Local AI ---');
        console.log(data.response);
    } catch (error) {
        console.error('❌ Lỗi:', error.message);
        console.log('💡 Gợi ý: Chắc chắn bạn đã mở Ollama và tải model bằng lệnh: ollama run gemma2:2b');
    }
}

askLocalAI('Giải thích cơ chế Event Loop trong Node.js cho người mới bắt đầu ngắn gọn trong 150 từ.');
