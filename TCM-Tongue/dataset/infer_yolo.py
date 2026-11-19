import sys
import json
from pathlib import Path
from ultralytics import YOLO

BASE_DIR = Path(__file__).resolve().parent
ROOT_DIR = BASE_DIR.parent.parent

# 1. Lấy ảnh từ tham số CLI; nếu bấm Run trực tiếp trong VS Code thì tự lấy 1 ảnh mẫu trong tập test
if len(sys.argv) > 1:
    image_path = Path(sys.argv[1])
else:
    sample_images = sorted((BASE_DIR / "shezhenv3-coco-percent-3" / "test" / "images").glob("*.jpg"))
    if not sample_images:
        raise FileNotFoundError("Không tìm thấy ảnh đầu vào. Hãy truyền đường dẫn ảnh qua dòng lệnh.")
    image_path = sample_images[0]

# 2. Tự động tìm tệp best.pt mới nhất ở cả 2 thư mục lưu trữ mặc định của Ultralytics
candidates = list(BASE_DIR.glob("TCM_Tongue_Runs/**/weights/best.pt")) + list(
    ROOT_DIR.glob("runs/detect/**/weights/best.pt")
)
if not candidates:
    raise FileNotFoundError(
        "Chưa có tệp best.pt. Hãy chạy 'python TCM-Tongue/dataset/train.py' trước để tạo checkpoint."
    )

weights_path = max(candidates, key=lambda p: p.stat().st_mtime)

model = YOLO(str(weights_path))
results = model.predict(source=str(image_path), conf=0.25, iou=0.6, verbose=False)

detections = []
for r in results:
    for box in r.boxes:
        cls_id = int(box.cls[0])
        detections.append({
            "label": model.names[cls_id],
            "confidence": round(float(box.conf[0]), 4),
            "bbox": [round(x, 1) for x in box.xyxy[0].tolist()]
        })

output = {
    "weights_used": str(weights_path),
    "image": str(image_path),
    "detections": detections
}
print(json.dumps(output, ensure_ascii=False, indent=2))