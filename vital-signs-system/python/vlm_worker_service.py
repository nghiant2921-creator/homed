# Thêm vào phần import của vlm_worker_service.py
from fastapi.responses import HTMLResponse
from fastapi.staticfiles import StaticFiles

# ... (giữ nguyên phần khởi tạo app, socket_listener_loop, /pending, /review/{event_id}) ...

# 1. Mount thư mục ảnh bằng chứng để trình duyệt tải ảnh trực tiếp (chỉ đọc)
app.mount("/evidence", StaticFiles(directory=ARCHIVE_DIR), name="evidence")

# 2. Phục vụ giao diện kiểm duyệt HITL tại đường dẫn gốc "/"
@app.get("/", response_class=HTMLResponse)
def serve_hitl_dashboard():
    with open("hitl_dashboard.html", "r", encoding="utf-8") as f:
        return f.read()