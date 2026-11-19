from ultralytics.data.converter import convert_coco

convert_coco(
    labels_dir="./TCM-Tongue/dataset/shezhenv3-coco-percent-3/train/annotations",  # Thư mục chứa file .json
    save_dir="./TCM-Tongue-YOLO",
    use_segments=False
)