# Vital Signs component sync

Checked on 2026-11-26.

- The C sources and Python baseline inspector were moved from `src/` into
  `vital-signs-system/`; each new file is byte-identical to its previously
  tracked counterpart.
- A standalone doctor-facing preview is available at
  `vital-signs-system/doctor_dashboard.html`. It starts with synthetic demo
  data and can import daemon JSONL locally; it is not a live feed and does not
  capture clinician accept/reject feedback for a human-in-the-loop workflow.
- Vital Signs is not ready for ML training: the C inference weights are fixed
  constants, `inspect_baseline.py` only reads a baseline profile, and there is
  no Vital Signs training script or labelled dataset in this directory. The
  daemon's adaptive baseline is calibration, not model training.
- The Windows `.exe` was moved with the component. Ubuntu users should compile
  native binaries using `vital-signs-system/README.md`.
- Python AST parsing passed. The available Windows GCC returned exit code 1
  without diagnostics for both C syntax checks, so C compilation was not
  verified in this environment.
- Dashboard browser check passed for demo rendering and JSONL import, including
  displaying an alert flag and skipping a `WARMING_UP` record.
- Root branch `master` was fetched from GitHub at `49a9fee`; no newer laptop
  commit was present on that remote at check time. GitHub's default branch is
  `main` at the initial commit, so Ubuntu clones must explicitly request
  `--branch master`.
- `.vscode/tasks.json` is ignored by the repository and uses a machine-local
  MSYS2 Windows GCC path; it was not staged.
- The repository API uses `@google/genai`; `.env` contains a `GEMINI_API_KEY`
  variable, but its value was not read or recorded. No secrets, datasets, or
  model checkpoints were staged.
- TCM training remains configured for a CPU smoke test (YOLO11n, 1 epoch,
  5% of data, 320px, batch 4, workers 0); the available PyTorch is CPU-only
  (`torch 2.15.0+cpu`, CUDA unavailable). The TCM dataset config defines 20
  classes.
- Fall detection continues to use its separately configured submodule and
  `yolo11n-pose.pt` local weight.
