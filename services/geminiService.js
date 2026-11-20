import { GoogleGenAI, Type } from "@google/genai";
import "dotenv/config";

export async function synthesizeWithGemini({ tongueResult, fallResult, userNote }) {
    if (!process.env.GEMINI_API_KEY) {
        return {
            api_used: false,
            message: "Chưa cấu hình GEMINI_API_KEY trong .env. Trả về kết quả nhận diện thô.",
            raw_cv: { tongueResult, fallResult },
        };
    }

    const ai = new GoogleGenAI({ apiKey: process.env.GEMINI_API_KEY });

    const prompt = `
Bạn là hệ thống hỗ trợ đánh giá sức khỏe kết hợp Y học cổ truyền (chẩn đoán lưỡi) và Thị giác máy tính (giám sát dáng đi / té ngã).
Hãy phân tích dữ liệu đầu vào sau:
- Kết quả mô hình TCM-Tongue (20 nhãn bệnh lý): ${JSON.stringify(tongueResult || "Không cung cấp")}
- Kết quả mô hình Fall-Detection (té ngã/dáng đi): ${JSON.stringify(fallResult || "Không cung cấp")}
- Ghi chú / triệu chứng người dùng mô tả: "${userNote || "Không có"}"

Yêu cầu:
1. Giải thích ý nghĩa các nhãn lưỡi phát hiện được theo YHCT (nếu có).
2. Đánh giá mức độ cảnh báo té ngã / bất thường dáng đi (nếu có).
3. Đưa ra nhận định tổng hợp mối liên hệ (ví dụ: thể trạng khí hư, thận hư trên lưỡi làm tăng nguy cơ mất thăng bằng) và khuyến nghị chăm sóc an toàn.
`;

    const response = await ai.models.generateContent({
        model: "gemini-2.5-flash",
        contents: prompt,
        config: {
            responseMimeType: "application/json",
            responseSchema: {
                type: Type.OBJECT,
                properties: {
                    alert_level: {
                        type: Type.STRING,
                        description: "Mức độ ưu tiên: LOW, MEDIUM, HIGH, hoặc CRITICAL (nếu phát hiện té ngã)",
                    },
                    tcm_syndrome_summary: {
                        type: Type.STRING,
                        description: "Tóm tắt biện chứng luận trị từ các đặc điểm lưỡi",
                    },
                    fall_gait_assessment: {
                        type: Type.STRING,
                        description: "Đánh giá trạng thái vận động, dáng đi và nguy cơ té ngã",
                    },
                    combined_recommendation: {
                        type: Type.STRING,
                        description: "Lời khuyên điều dưỡng YHCT kết hợp phòng ngừa té ngã",
                    },
                },
                required: [
                    "alert_level",
                    "tcm_syndrome_summary",
                    "fall_gait_assessment",
                    "combined_recommendation",
                ],
            },
        },
    });

    return {
        api_used: true,
        model: "gemini-2.5-flash",
        analysis: JSON.parse(response.text),
    };
}