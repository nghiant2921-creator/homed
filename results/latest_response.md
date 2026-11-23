# GitHub push and GPU workstation status

- The local repository is `master`, with `origin` set to `https://github.com/nghiant2921-creator/homed.git`.
- Verified GitHub `refs/heads/master` is at commit `832f04f1d96431d57535c25718357607a6383305`, matching the latest local commit at verification time. The project is pushed; no additional push command is needed unless new commits are made.
- To push future changes from the project directory:

  ```bash
  git add .
  git commit -m "Describe your change"
  git push
  ```

- `.env`, Python environments, `node_modules`, dataset archives/extracted images, weights, and training runs are ignored by Git. Never commit the Gemini API key.
- Dataset archives are several gigabytes and are not included. Obtain the data separately from its original source, respecting its license and the attribution in `TCM-Tongue/README.md`.
- The current local model is a CPU smoke-test checkpoint (`torch 2.15.0+cpu`, `CUDA=False`), not a full GPU-trained model. Training uses the 20 classes in `tcm_tongue.yaml`. On an NVIDIA workstation, install a CUDA-compatible PyTorch build, verify `torch.cuda.is_available()` is `True`, install `requirements-training.txt`, put the dataset in the documented directory, then run `python TCM-Tongue/dataset/train.py`.

## Edge Triage C daemon

- The C source now supports native Windows testing and Ubuntu POSIX.
- Native Windows uses UDP loopback (`127.0.0.1:9001`) and UDP alerts (`127.0.0.1:9002`); `--fifo` remains POSIX-only.
- Verified native build and 12-frame inference smoke test with MSYS2/UCRT64 GCC and Winsock 2. The executable starts, warms up after 10 frames, emits `status:"OK"` JSON, and writes a 56-byte baseline profile atomically.
- Ubuntu 24.04 build:

  ```bash
  gcc -std=c11 -O2 -Wall -Wextra src/CCplusCsharp/edge_triage_daemon.c -lm -o edge_triage_daemon
  ```

- Run in WSL with UDS defaults or explicit `--uds`, `--fifo`, `--alert-uds`, `--profile`, `--subject-id`, `--init-hr`, and `--init-spo2` options.
- Gemini API and fall-detection model weights are not required to build or run this C daemon.

## Python baseline inspection on Windows

- The Code Runner log calls `python -u`; PowerShell resolves `python` to the WindowsApps stub, while the Python 3.11 launcher and project `.venv` are installed.
- `src/python/inspect_baseline.py` now accepts an optional profile path and defaults to `C:\Temp\patient_baseline.bin` on Windows (`/tmp/patient_baseline.bin` on Linux).
- Verified with `py -3.11 src\python\inspect_baseline.py` and the workspace venv; it reads the 56-byte profile and validates its CRC.

## GDB output: edge_vital_triage

- The debugger launched `src/CCplusCsharp/edge_vital_triage.exe`, built from `edge_vital_triage.c`, not the persistent `edge_triage_daemon.c`.
- In `edge_vital_triage.c`, `main(void)` simulates 20 telemetry frames, emits predictions after the 10-frame buffer fills (ticks 9-19), then returns 0. GDB's "exited normally" is expected, not a crash.
- Ran the debugger-targeted executable directly: it printed 11 JSON prediction lines and exited with code 0.
- To run a persistent sensor daemon, build and launch `edge_triage_daemon.c` / `edge_triage_daemon.exe` instead; it waits for input until Ctrl+C.
- TCM training remains CPU smoke-test configuration when CUDA is unavailable, with 20 configured classes and no training-run checkpoint found. The fall-detection folder has no model/README in this workspace. The Gemini key variable exists in `.env`, but its value is not needed for this C test.

## Git command typo

- The PowerShell command `gid add .` failed because `gid` is not a command; the Git command is `git add .`.
- `.env` is ignored by Git. `git status --short` shows multiple untracked paths, so review them before staging everything. No files were staged automatically.
