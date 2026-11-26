# Vital Signs Edge Component

This directory contains a C edge-triage simulator, a C telemetry daemon, and
a Python utility for inspecting the daemon's patient baseline profile.

## Build on Ubuntu 24.04

Install GCC if needed:

```bash
sudo apt update
sudo apt install -y build-essential python3
```

Build and run the standalone 20-second simulator:

```bash
gcc -std=c11 -O2 -Wall -Wextra \
  CCplusCsharp/edge_vital_triage.c -lm -o /tmp/edge_vital_triage
/tmp/edge_vital_triage
```

Build the telemetry daemon:

```bash
gcc -std=c11 -O2 -Wall -Wextra \
  CCplusCsharp/edge_triage_daemon.c -lm -o /tmp/edge_triage_daemon
```

The daemon uses `/tmp/vital_triage.sock`, `/tmp/vital_alerts.sock`, and
`/tmp/patient_baseline.bin` by default. Override socket and profile paths with
`--uds`, `--alert-uds`, and `--profile`; use `--subject-id`, `--init-hr`, and
`--init-spo2` to set the initial profile values.

Inspect a baseline profile written by the daemon:

```bash
python3 python/inspect_baseline.py /tmp/patient_baseline.bin
```

## Doctor-facing dashboard preview

Open `doctor_dashboard.html` directly in a browser. It starts with clearly
labelled synthetic demo data and can import JSONL output from the C daemon.
To save the daemon's JSON output locally while it runs:

```bash
/tmp/edge_triage_daemon \
  --uds /tmp/vital_triage.sock \
  --alert-uds /tmp/vital_alerts.sock \
  --profile /tmp/patient_baseline.bin \
  > /tmp/vital-telemetry.jsonl
```

After collecting data, use **Nạp JSONL từ daemon** in the page to select
`/tmp/vital-telemetry.jsonl`. The page parses the file locally; it does not
connect to the daemon or upload the data. `WARMING_UP` lines are skipped.
This is a display/import preview only: it does not capture clinician
accept/reject feedback, provide a live feed, or train the model.

The profile inspector uses only the Python standard library. The committed
`.exe` file is a Windows binary; build a native executable on Ubuntu with the
commands above instead.

## Training readiness

The current Vital Signs C programs contain fixed inference weights; the Python
script only reads and validates a baseline profile. There is not yet a
Vital Signs training script or labelled training dataset in this directory.
The daemon's adaptive patient baseline is calibration, not model-weight
training. Prepare a consented, de-identified telemetry dataset with labels and
a training/export pipeline before attempting model training.

This is a proof-of-concept component, not a medical device or a substitute for
clinical assessment. Validate it with appropriate data and hardware before
real-world use.
