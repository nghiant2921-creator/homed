# Doctor dashboard and Ubuntu backend setup

Checked on 2026-11-26.

- The React/Vite app now opens the doctor-facing Vital Signs dashboard from
  `vital-signs-system/doctor_dashboard.html` as its first screen.
- The dashboard currently displays demo telemetry; JSONL imports are processed
  locally in the browser. The UI is not connected to the Express API yet.
- The top bar links to the official Gemini API documentation. It does not send
  dashboard data to Gemini.
- Replacing that link with Ubuntu API output is pending the backend URL/route
  and a sanitized sample response. Existing analyze routes are POST endpoints
  that accept uploaded media, so the dashboard should call them and render JSON
  rather than navigate to them as ordinary links.
- Verified `npm run build` succeeds and the dev page renders the dashboard and
  Gemini API link at `http://127.0.0.1:5173/`.
- The frontend has `npm run dev`, `npm run build`, and `npm run preview`.
- A GitHub Actions workflow now builds the static doctor dashboard and deploys
  it to GitHub Pages on pushes to `master`. Expected public URL:
  `https://nghiant2921-creator.github.io/homed/`. A direct check currently
  returns 404; the workflow must be pushed and GitHub Pages set to GitHub
  Actions under Settings → Pages before the address becomes available.
- The hosted page is a demo-only static frontend: it cannot access the Ubuntu
  backend or run Gemini analysis. Never publish or load real patient data on
  the public site.
- Vite proxies `/api/*` to `VITE_BACKEND_URL`, defaulting to
  `http://localhost:3000`. Put the Ubuntu private-network address in the
  ignored `.env.local` on Windows.
- The existing backend routes are `/api/analyze/tongue`, `/api/analyze/fall`,
  and `/api/analyze/holistic`; Gemini is called by the backend service.
- The local `.env` has a `GEMINI_API_KEY` entry; only its presence was checked,
  never its value. Keep the key on the backend, not in a `VITE_` variable.
- Keep `GEMINI_API_KEY` only in the Ubuntu backend environment. Do not prefix
  secrets with `VITE_`, because Vite exposes those variables to browser code.
- Latest TCM run `yolo11_tcm-5` is a CPU smoke test (YOLO11n, 1 epoch,
  5% of data, 320px, batch 4, workers 0); the local PyTorch build is
  CPU-only (`torch 2.15.0+cpu`, CUDA unavailable). The YAML defines 20 classes;
  local `best.pt` and `last.pt` checkpoints exist, but this was only a smoke
  test and not full GPU training.
- The fall detector has its local `yolo11n-pose.pt` inference weight and
  inference/benchmark scripts; the inspected folder has no training script.
  Training artifacts and model weights are excluded from Git.
