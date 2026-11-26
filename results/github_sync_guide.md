# GitHub synchronization guide

Checked against GitHub on 2026-11-26.

## Repository status

- Root repository: `nghiant2921-creator/homed`, branch `master`.
- Root previously synchronized to `7f2b5fe` (`adding vital signs`); this update adds submodule metadata and this guide.
- The nested repository uses `https://github.com/nghiant2921-creator/fall-detection-system.git`, branch `main`. Commit `f2aa309` publishes `infer_fall.py` and `requirements-cpu.txt`.
- `.gitmodules` now maps `fall-detection-system` to that GitHub repository so a recursive clone on Ubuntu can initialize it.
- `.env` exists and has a `GEMINI_API_KEY` entry; the key value is intentionally not recorded. The root does not track `.env`.

## Safe root synchronization

From the root repository in the VS Code terminal:

```powershell
git status --short --branch
git fetch origin
git log --oneline --left-right HEAD...origin/master
git pull --ff-only origin master
git status --short --branch
```

`--ff-only` prevents Git from creating a merge commit. The newly published report and submodule pointer are committed separately to root `master`.

## Nested fall-detection repository

To get both repositories on Ubuntu 24.04:

```powershell
git clone --recurse-submodules https://github.com/nghiant2921-creator/homed.git
cd homed
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r fall-detection-system/requirements-cpu.txt
```

The nested repository is separately pushable from `fall-detection-system`; the root tracks its selected commit as a submodule.

## Keep local-only data out of Git

The TCM run configuration records a CPU smoke test: YOLOv11n, one epoch, 5% of the data, 320px images, batch 4, and zero workers. Its `best.pt` and `last.pt` are local checkpoints. The fall detector currently has `yolo11n-pose.pt` and no separate trained fall checkpoint. The root ignore rules exclude `.env`, `.pt` weights, datasets, and training runs; keep these local and do not force-add them.

The project uses `@google/genai` and has `/api/analyze/tongue` and `/api/analyze/fall` routes. Never include API key values in Git commits or support reports.
