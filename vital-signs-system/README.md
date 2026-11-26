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

The profile inspector uses only the Python standard library. The committed
`.exe` file is a Windows binary; build a native executable on Ubuntu with the
commands above instead.

This is a proof-of-concept component, not a medical device or a substitute for
clinical assessment. Validate it with appropriate data and hardware before
real-world use.
