# Quy tắc thực thi bắt buộc cho dự án TCM-Tongue

Mỗi khi nhận được prompt trong workspace này, hãy thực hiện tuần tự 4 bước sau trước khi đưa ra câu trả lời cuối cùng:

1. **Tham khảo cấu hình & trạng thái Training trước:**
   - Đọc `TCM-Tongue/dataset/train.py` và `TCM-Tongue/dataset/tcm_tongue.yaml` (20 nhãn triệu chứng lưỡi YHCT).
   - Kiểm tra thư mục `TCM-Tongue/dataset/TCM_Tongue_Runs/` (nếu đã có `results.csv` hoặc `weights/best.pt`) để xác định mô hình đang chạy thử trên CPU hay đã huấn luyện xong trên GPU.

2. **Kiểm tra API hiện có trong dự án:**
   - Kiểm tra `package.json`, `.env` (chỉ kiểm tra sự tồn tại của biến `GEMINI_API_KEY` hoặc cấu hình endpoint, không in giá trị key ra ngoài) và các tệp `.js`/`.py` trong thư mục gốc.
   - Nếu dự án đã có sẵn API/SDK (`@google/genai`, Express/Fastify route...), ưu tiên tích hợp trực tiếp vào kiến trúc hiện có. Nếu chưa có hoặc key trống, cung cấp giải pháp chạy độc lập cục bộ.

3. **Trả lời đúng ngữ cảnh phần cứng & dữ liệu:**
   - Đảm bảo mã nguồn tương thích chéo giữa máy văn phòng (CPU không có AVX2, `CUDA=False`) và máy GPU sau này, bám sát đúng 20 lớp bệnh lý trong `tcm_tongue.yaml`.

4. **Lưu kết quả vào thư mục `results/`:**
   - Tự động tạo hoặc cập nhật kết quả đầu ra (báo cáo phân tích, cấu hình cập nhật, hoặc kết quả suy luận mẫu) vào thư mục `results/` (ví dụ: `results/latest_response.md` hoặc `results/output.json`).