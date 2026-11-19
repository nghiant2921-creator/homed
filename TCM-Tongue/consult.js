import { execFileSync } from "child_process";
import fs from "fs";
import path from "path";
import "dotenv/config";

const imagePath = process.argv[2] || "sample_tongue.jpg";
const userPrompt = process.argv[3] || "Phân tích tình trạng lưỡi và gợi ý hướng điều dưỡng theo YHCT.";

// 1. Tham khảo mô hình YOLO đã train trước
const pythonBin = path.join(".venv", "Scripts", "python.exe");
const rawOutput = execFileSync(pythonBin, ["infer_yolo.py", imagePath], { encoding: "utf-8" });
const yoloDetections = JSON.parse(rawOutput);

let finalAnswer = "";

// 2. Kiểm tra nếu có GEMINI_API_KEY thì gọi API để luận giải chuyên sâu
if (process.env.GEMINI_API_KEY) {
    const { GoogleGenAI } = await import("@google/genai");
    const ai = new GoogleGenAI({ apiKey: process.env.GEMINI_API_KEY });

    const prompt = `
Dữ liệu nhận diện từ mô hình YOLOv11 (TCM-Tongue): ${JSON.stringify(yoloDetections)}
Câu hỏi người dùng: ${userPrompt}
Hãy đối chiếu các nhãn triệu chứng YHCT trên và đưa ra nhận định tham khảo ngắn gọn, rõ ràng.
  `;

    const response = await ai.models.generateContent({
        model: "gemini-2.5-flash",
        contents: prompt,
    });
    finalAnswer = response.text;
} else {
    finalAnswer = "Chưa cấu hình GEMINI_API_KEY trong .env. Chỉ trả về kết quả nhận diện thô từ mô hình YOLO.";
}

// 3. Lưu kết quả ra thư mục results/
fs.mkdirSync("results", { recursive: true });
const record = {
    timestamp: new Date().toISOString(),
    image: imagePath,
    prompt: userPrompt,
    yolo_detections: yoloDetections,
    ai_consultation: finalAnswer,
};

const outFile = path.join("results", `result_${Date.now()}.json`);
fs.writeFileSync(outFile, JSON.stringify(record, null, 2), "utf-8");

console.log("Kết quả:", finalAnswer);
console.log("Đã lưu tại:", outFile);