# Quy tắc thực thi bắt buộc cho dự án (TCM-Tongue & Fall Detection)

Mỗi khi nhận được prompt trong workspace này, hãy thực hiện tuần tự 4 bước sau trước khi đưa ra câu trả lời cuối cùng:

1. **Tham khảo cấu hình & trạng thái Training của cả 2 component trước:**
   - **Component 1 (Chẩn đoán lưỡi YHCT):** Đọc `TCM-Tongue/dataset/train.py`, `TCM-Tongue/dataset/tcm_tongue.yaml` (20 lớp bệnh lý) và kiểm tra checkpoint mới nhất trong `TCM-Tongue/dataset/TCM_Tongue_Runs/` hoặc `runs/detect/`.
   - **Component 2 (Dự đoán té ngã / dáng đi):** Đọc cấu trúc thư mục, tệp `README.md`, script huấn luyện/suy luận và trọng số mô hình (`.pt`, `.onnx`, `.h5`...) trong `fall-detection-system/`.
   - Xác định rõ trạng thái hiện tại (đang chạy thử nghiệm trên máy văn phòng CPU không có AVX2/`CUDA=False` hay đã train trên máy GPU).

2. **Kiểm tra API hiện có trong dự án:**
   - Kiểm tra `package.json`, `.env` (chỉ kiểm tra sự tồn tại của biến `GEMINI_API_KEY` hoặc các biến API khác, tuyệt đối không in giá trị key) và các endpoint trong thư mục gốc.
   - Tận dụng trực tiếp SDK/API đang có sẵn (`@google/genai`...) khi tích hợp luồng chẩn đoán hoặc cảnh báo.

3. **Trả lời đúng ngữ cảnh phần cứng & dữ liệu:**
   - Đảm bảo mã nguồn tương thích chéo giữa máy văn phòng (CPU yếu, cần `polars[rtcompat]`, `workers=0` hoặc `2`) và máy GPU sau này.

4. **Lưu kết quả vào thư mục `results/`:**
   - Tự động ghi nhận báo cáo, cấu hình mới hoặc kết quả suy luận mẫu của cả 2 component vào thư mục `results/` (ví dụ: `results/fall_detection_audit.md`, `results/latest_response.md`).