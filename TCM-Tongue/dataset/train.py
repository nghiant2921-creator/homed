from pathlib import Path
from tempfile import TemporaryDirectory
import torch
from ultralytics import YOLO
from ultralytics.utils import YAML

BASE_DIR = Path(__file__).resolve().parent
DATA_YAML = BASE_DIR / "tcm_tongue.yaml"
HAS_GPU = torch.cuda.is_available()

# Máy văn phòng (CPU) dùng bản nhẹ & 5% data để test code; máy GPU chạy full cấu hình
model = YOLO("yolo11s.pt" if HAS_GPU else "yolo11n.pt")

data_config = YAML.load(DATA_YAML)
dataset_root = Path(data_config["path"])
if not dataset_root.is_absolute():
    dataset_root = BASE_DIR / dataset_root
data_config["path"] = str(dataset_root.resolve())

with TemporaryDirectory(prefix="tcm_tongue_") as temp_dir:
    runtime_data_yaml = Path(temp_dir) / DATA_YAML.name
    YAML.save(runtime_data_yaml, data_config)

    results = model.train(
        data=str(runtime_data_yaml),
        epochs=100 if HAS_GPU else 1,
        fraction=1.0 if HAS_GPU else 0.05,   # CPU chỉ chạy thử 5% dữ liệu
        imgsz=640 if HAS_GPU else 320,
        batch=16 if HAS_GPU else 4,
        workers=8 if HAS_GPU else 0,          # Windows CPU yếu nên để workers=0 tránh tràn RAM
        patience=20,
        optimizer="AdamW",
        lr0=0.001,
        flipud=0.0,
        fliplr=0.5,
        hsv_h=0.005,
        hsv_s=0.3,
        hsv_v=0.3,
        mosaic=0.5,
        project=str(BASE_DIR / "TCM_Tongue_Runs"),
        name="yolo11_tcm"
    )