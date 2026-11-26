# Edge Triage Daemon - huong dan build va chay tren Windows

Ngay kiem tra: 2026-11-26

## Ket luan

File `vital-signs-system/CCplusCsharp/edge_triage_daemon.c` hien co the build va chay native tren Windows.
Windows su dung UDP loopback thay cho UDS/FIFO; Ubuntu van su dung UDS/FIFO POSIX.

API POSIX tren Linux:

- `AF_UNIX`, `SOCK_DGRAM`, `bind`, `sendto`, `recvfrom`
- `poll`
- `unistd.h`
- `sys/un.h`
- FIFO va POSIX signal

Ban Windows native da duoc bo sung cac phien ban Winsock va khong can WSL de test.
WSL 2/Ubuntu van la moi truong san sang cho kich ban UDS/FIFO.

## Cai tren Windows

Co the dung MSYS2/UCRT64 GCC da co san tren laptop:

```powershell
& 'D:\msys64\ucrt64\bin\gcc.exe' -std=c11 -O2 -Wall -Wextra `
  vital-signs-system/CCplusCsharp/edge_triage_daemon.c `
  -lws2_32 -o edge_triage_daemon.exe
```

## Build

### Windows native

Da test thanh cong voi MSYS2/UCRT64 GCC va Winsock 2. File executable:
`edge_triage_daemon.exe`.

### Ubuntu

Trong Ubuntu:

```bash
cd /mnt/d/htmlcss-course/reactJS/my-chatBot-app
gcc -std=c11 -O2 -Wall -Wextra \
  vital-signs-system/CCplusCsharp/edge_triage_daemon.c \
  -lm -o edge_triage_daemon
```

Khong can Node.js, Python, PyTorch, CUDA, OpenCV hay Gemini API de build va chay daemon C.

## Cau hinh va chay

### Windows native (da test)

```powershell
.\edge_triage_daemon.exe --uds 127.0.0.1:9001 --alert-uds 127.0.0.1:9002 --profile C:\Temp\patient_baseline.bin --init-hr 75 --init-spo2 97
```

Gui 12 frame:

```powershell
py -3 -c "import socket,time; s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); [(s.sendto(f'{75+i%4} {98-i%2} {20+i%3} 0.5 0.1'.encode(),('127.0.0.1',9001)),time.sleep(.1)) for i in range(12)]"
```

Sau 10 frame se bat dau tra ve `status:"OK"` thay vi `WARMING_UP`.

Doc profile tren Windows:

```powershell
py -3.11 -u vital-signs-system\python\inspect_baseline.py
```

Script mac dinh doc `C:\Temp\patient_baseline.bin` tren Windows va
`/tmp/patient_baseline.bin` tren Linux. Co the truyen duong dan khac lam doi so thu nhat.

### Ubuntu POSIX

Mac dinh daemon dung UDS:

```bash
./edge_triage_daemon \
  --uds /tmp/vital_triage.sock \
  --alert-uds /tmp/vital_alerts.sock \
  --profile /tmp/patient_baseline.bin \
  --subject-id 1001 \
  --init-hr 75 \
  --init-spo2 97
```

Gui mot mau cam bien bang Python 3:

```bash
python3 -c "import socket; s=socket.socket(socket.AF_UNIX,socket.SOCK_DGRAM); s.sendto(b'75 98 20 0.5 0.1', '/tmp/vital_triage.sock')"
```

Dinh dang telemetry: `nhip_tim SpO2 goc_than chieu_cao_hong toc_do_khop`.
Moi dong/cac datagram co 5 so.

Che do FIFO:

```bash
./edge_triage_daemon --fifo /tmp/vital.pipe --profile /tmp/patient_baseline.bin
```

Sau do terminal khac:

```bash
printf '75 98 20 0.5 0.1\n' > /tmp/vital.pipe
```

Theo doi JSON dau ra tren stdout va duong dan profile. Ctrl+C se luu profile va xoa socket vao.

## Trang thai component lien quan

- TCM-Tongue: dataset 20 lop duoc dinh nghia trong `TCM-Tongue/dataset/tcm_tongue.yaml`.
- Training gan nhat la CPU smoke-test: YOLO11n, 1 epoch, 5% du lieu, kich thuoc anh 320, batch 4, workers 0. Checkpoint `best.pt` va `last.pt` co trong `TCM-Tongue/dataset/TCM_Tongue_Runs/yolo11_tcm-5/weights/`; PyTorch hien la CPU-only (`torch 2.15.0+cpu`, `CUDA=False`).
- GPU workstation can Python 3.11, CUDA-capable PyTorch va Ultralytics; khong lien quan truc tiep den daemon C nay.
- `.env` co bien `GEMINI_API_KEY`; gia tri khong duoc kiem tra. Key khong can thiet cho `edge_triage_daemon.c`.
- `fall-detection-system/` la submodule rieng, co README va trong so `yolo11n-pose.pt` trong workspace.

## Trang HTML theo doi cho bac si

- `index.html` o thu muc goc la diem mount cua Vite (`#root` va `/src/main.jsx`), khong phai trang dashboard.
- `src/App.jsx` hien la giao dien mau Vite/React. Khong tim thay trang cu trong cac commit hoac snapshot Git con luu; trang da duoc tao lai rieng tai `vital-signs-system/doctor_dashboard.html`.
- Trang moi hien du lieu minh hoa mac dinh va co the nap JSONL do daemon C ghi ra; no khong ket noi truc tiep voi daemon va khong tu dong cap nhat theo thoi gian thuc.
- Du lieu JSONL duoc xu ly trong trinh duyet, khong gui len may chu. Trang hien thi telemetry va nhan mo hinh, khong phai ket luan y khoa.
- Truoc khi tao lai trang, lich su Git chi ghi nhan `index.html` trong commit `7f2b5fe` (`adding vital signs`); khong tim thay dashboard cu trong cac commit hoac snapshot Git con luu.
- Cach mo trang va nap JSONL duoc ghi trong `vital-signs-system/README.md`.
