# Chuan bi project de push GitHub va chay tren GPU workstation

- Da khoi tao Git repository local; workspace truoc do chua co `.git`.
- Da them `.gitignore` de loai `.env`, virtualenv, `node_modules`, weights `.pt`, cac run, dataset archive/extracted data va ket qua suy luan theo tung anh.
- Dataset archive trong workspace nang khoang 3.1 GB va 873 MB; khong nen dua len GitHub. Can lay dataset tu nguon goc theo ghi chu attribution trong `TCM-Tongue/README.md` va dam bao quyen su dung.
- Da them `README.md` voi huong dan Windows/Linux, cai PyTorch CUDA theo workstation, cai Ultralytics, xac minh `torch.cuda.is_available()` va chay script train chung.
- Da them `requirements-training.txt` va `.env.example`; `.env` thuc te duoc ignore, khong in hoac dua key len Git.
- Trang thai model hien tai: PyTorch `2.15.0+cpu`, CUDA=False; co checkpoint local tu CPU smoke-test, khong phai model GPU da huan luyen day du.
- Training config van giu 20 class; tren CUDA bat se chay cau hinh GPU 100 epochs/fraction=1.0, tren CPU chay smoke test.
- Da `git init` va kiem tra `.gitignore` nhan dien dung `.env`, archives, dataset, weights va node_modules.
- Commit chua tao vi Git chua co author name/email; cac file dang staged, khong co `.env`, weights, hoac dataset.
- Nguoi dung da cung cap commit author name/email va yeu cau public repository moi, nhung chuoi nhap `cumminity-paliative` can xac nhan la ten repo chinh xac.
- Chua tao remote/commit/push den khi co ten repo va URL chinh xac; profile URL khong phai clone URL.
