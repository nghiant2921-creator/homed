import { GoogleGenAI } from '@google/genai';

const apiKey = process.env.GEMINI_API_KEY;

async function generateText() {
    // Nếu chưa có key thật, trả về kết quả giả lập
    if (!apiKey || apiKey === 'AIzaSyYourActualApiKeyHere') {
        console.log('⚠️ [CHẾ ĐỘ TẠM THỜI]: Chưa có API key thật, sử dụng phản hồi giả lập (Mock).');
        console.log('--- Phản hồi từ Gemini ---');
        console.log('Event Loop là cơ chế đơn luồng giúp Node.js xử lý bất đồng bộ không bị nghẽn (non-blocking I/O)...');
        return;
    }

    try {
        const ai = new GoogleGenAI({});
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