import sys
import json
import torch
from pathlib import Path
from ultralytics import YOLO

BASE_DIR = Path(__file__).resolve().parent
ROOT_DIR = BASE_DIR.parent.parent

def find_latest_weights():
    candidates = list(BASE_DIR.glob("TCM_Tongue_Runs/**/weights/best.pt")) + list(
        ROOT_DIR.glob("runs/detect/**/weights/best.pt")
    )
    if not candidates:
        return None
    return max(candidates, key=lambda p: p.stat().st_mtime)

def main():
    if len(sys.argv) < 2:
        print(json.dumps({"error": "Thiếu đường dẫn ảnh lưỡi đầu vào"}))
        sys.exit(1)

    image_path = Path(sys.argv[1])
    weights_path = find_latest_weights()

    if weights_path is None:
        print(json.dumps({
            "component": "tcm_tongue",
            "status": "NO_WEIGHTS",
            "message": "Chưa tìm thấy best.pt. Hãy chạy train.py trước.",
            "detections": []
        }, ensure_ascii=False))
        return

    device = 0 if torch.cuda.is_available() else "cpu"
    model = YOLO(str(weights_path))
    results = model.predict(source=str(image_path), conf=0.25, iou=0.6, device=device, verbose=False)

    detections = []
    for r in results:
        for box in r.boxes:
            cls_id = int(box.cls[0])
            detections.append({
                "label": model.names[cls_id],
                "confidence": round(float(box.conf[0]), 4),
                "bbox": [round(x, 1) for x in box.xyxy[0].tolist()]
            })

    print(json.dumps({
        "component": "tcm_tongue",
        "status": "OK",
        "device": str(device),
        "weights": weights_path.name,
        "detections": detections
    }, ensure_ascii=False))

if __name__ == "__main__":
    main()