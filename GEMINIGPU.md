# Project Context & Execution Rules

Mỗi khi nhận được yêu cầu (prompt) từ người dùng trong dự án này, bắt buộc thực hiện theo đúng trình tự 4 bước sau:

## Bước 1: Tham khảo cấu hình & kết quả Training trước
- Luôn kiểm tra tệp cấu hình huấn luyện tại `TCM-Tongue/dataset/train.py` và `TCM-Tongue/dataset/tcm_tongue.yaml` (20 nhãn triệu chứng lưỡi Y học cổ truyền).
- Kiểm tra xem đã có trọng số huấn luyện mới nhất tại `TCM-Tongue/dataset/TCM_Tongue_Runs/*/weights/best.pt` hay chưa, kèm các chỉ số trong `results.csv` (nếu có) để nắm rõ trạng thái mô hình hiện tại (đang chạy thử CPU hay đã train xong trên GPU).

## Bước 2: Kiểm tra API hiện có trong dự án
- Quét thư mục gốc (`package.json`, `.env`, các tệp `.js`/`.ts`/`.py`) để kiểm tra xem dự án đã cấu hình **Gemini API** (`GEMINI_API_KEY`, `@google/genai` hoặc `@google/generative-ai`) hay các endpoint backend nào chưa.
- Nếu có API: Mọi đề xuất code tích hợp hoặc suy luận chẩn đoán phải tận dụng trực tiếp SDK/API đang có sẵn trong dự án thay vì viết lại từ đầu.
- Nếu chưa có API key hoạt động: Cung cấp phương án chạy độc lập bằng mô hình YOLO cục bộ và ghi chú rõ chỗ cần điền API key.

## Bước 3: Trả lời bám sát ngữ cảnh
- Đưa ra giải pháp, đoạn mã hoặc phân tích phù hợp với phần cứng hiện tại (máy văn phòng CPU hoặc máy GPU) và đúng với 20 lớp bệnh lý của dataset TCM-Tongue.

## Bước 4: Lưu kết quả thực thi
- Mọi kết quả phân tích, cấu hình mới, hoặc đầu ra suy luận mẫu cần được lưu có cấu trúc vào thư mục `results/` (ví dụ: `results/training_notes.md` hoặc `results/inference_output.json`) để giữ lịch sử xuyên suốt khi chuyển từ máy văn phòng sang máy GPU.