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
