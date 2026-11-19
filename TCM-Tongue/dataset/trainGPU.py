from pathlib import Path

from ultralytics import YOLO

# Chọn trọng số khởi tạo:
# - YOLOv11: "yolo11n.pt" (nhẹ), "yolo11s.pt" (cân bằng), "yolo11m.pt" (chính xác cao)
# - YOLOv8:  "yolov8n.pt", "yolov8s.pt", "yolov8m.pt"
model = YOLO("yolo11s.pt")

results = model.train(
    data=Path(__file__).with_name("tcm_tongue.yaml").resolve(),
    epochs=100,
    imgsz=640,
    batch=16,               # Tăng lên 32 nếu GPU có >= 16GB VRAM
    patience=20,            # Dừng sớm nếu val mAP không cải thiện sau 20 epochs
    optimizer="AdamW",
    lr0=0.001,
    # Tinh chỉnh Augmentation phù hợp chẩn đoán lưỡi:
    flipud=0.0,             # Không lật ngược trên-dưới
    fliplr=0.5,             # Lật trái-phải bình thường
    hsv_h=0.005,            # Giảm nhiễu tông màu (mặc định 0.015)
    hsv_s=0.3,              # Giảm nhiễu độ bão hòa màu (mặc định 0.7)
    hsv_v=0.3,              # Giảm nhiễu độ sáng (mặc định 0.4)
    mosaic=0.5,             # Giảm tỷ lệ ghép 4 ảnh để giữ chi tiết vết nứt/điểm đỏ
    project="TCM_Tongue_Runs",
    name="yolo11s_tcm"
)