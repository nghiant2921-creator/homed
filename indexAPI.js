import { GoogleGenAI } from '@google/genai';

// SDK tự động đọc biến môi trường GEMINI_API_KEY nếu không truyền tham số
const ai = new GoogleGenAI({});

async function generateText() {
    try {
        const response = await ai.models.generateContent({
            model: 'gemini-2.5-flash',
            contents: 'Giải thích cơ chế Event Loop trong Node.js cho người mới bắt đầu.',
            config: {
                systemInstruction: 'Bạn là kỹ sư Backend giàu kinh nghiệm. Trả lời súc tích, dùng ví dụ thực tế và giới hạn trong 150 từ.',
                temperature: 0.4,
            },
        });

        console.log('--- Phản hồi từ Gemini ---');
        console.log(response.text);
    } catch (error) {
        console.error('Lỗi khi gọi Gemini API:', error.message);
    }
}

generateText();