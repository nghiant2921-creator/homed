import express from "express";
import multer from "multer";
import cors from "cors";
import fs from "fs";
import path from "path";
import "dotenv/config";

import { runPythonScript } from "./services/pythonRunner.js";
import { synthesizeWithGemini } from "./services/geminiService.js";

const app = express();
app.use(cors());
app.use(express.json());

fs.mkdirSync("uploads", { recursive: true });
fs.mkdirSync("results", { recursive: true });

const upload = multer({ dest: "uploads/" });

function saveResultRecord(type, payload) {
  const filename = `${type}_${Date.now()}.json`;
  const outPath = path.join("results", filename);
  fs.writeFileSync(outPath, JSON.stringify(payload, null, 2), "utf-8");
  return outPath;
}

// 1. Endpoint chỉ chẩn đoán lưỡi TCM-Tongue
app.post("/api/analyze/tongue", upload.single("tongue_image"), async (req, res) => {
  try {
    if (!req.file) return res.status(400).json({ error: "Vui lòng gửi file 'tongue_image'" });

    const tongueResult = await runPythonScript("TCM-Tongue/dataset/infer_tongue.py", req.file.path);
    const geminiInsight = await synthesizeWithGemini({
      tongueResult,
      userNote: req.body.note,
    });

    const record = { timestamp: new Date().toISOString(), type: "tongue", tongueResult, geminiInsight };
    const savedTo = saveResultRecord("tongue", record);

    res.json({ saved_to: savedTo, ...record });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// 2. Endpoint chỉ kiểm tra té ngã / dáng đi
app.post("/api/analyze/fall", upload.single("gait_media"), async (req, res) => {
  try {
    if (!req.file) return res.status(400).json({ error: "Vui lòng gửi file 'gait_media' (ảnh/video)" });

    const fallResult = await runPythonScript("fall-detection-system/infer_fall.py", req.file.path);
    const geminiInsight = await synthesizeWithGemini({
      fallResult,
      userNote: req.body.note,
    });

    const record = { timestamp: new Date().toISOString(), type: "fall", fallResult, geminiInsight };
    const savedTo = saveResultRecord("fall", record);

    res.json({ saved_to: savedTo, ...record });
  } catch (err) {
    res.status(500).json({ error: err.message });
  }
});

// 3. Endpoint HỢP NHẤT: Chạy song song cả 2 mô hình và tổng hợp qua Gemini API
app.post(
  "/api/analyze/holistic",
  upload.fields([
    { name: "tongue_image", maxCount: 1 },
    { name: "gait_media", maxCount: 1 },
  ]),
  async (req, res) => {
    try {
      const tongueFile = req.files?.tongue_image?.[0];
      const gaitFile = req.files?.gait_media?.[0];

      if (!tongueFile && !gaitFile) {
        return res.status(400).json({ error: "Cần gửi ít nhất 'tongue_image' hoặc 'gait_media'" });
      }

      // Chạy song song 2 mô hình CV
      const [tongueResult, fallResult] = await Promise.all([
        tongueFile ? runPythonScript("TCM-Tongue/dataset/infer_tongue.py", tongueFile.path) : null,
        gaitFile ? runPythonScript("fall-detection-system/infer_fall.py", gaitFile.path) : null,
      ]);

      const geminiInsight = await synthesizeWithGemini({
        tongueResult,
        fallResult,
        userNote: req.body.note,
      });

      const record = {
        timestamp: new Date().toISOString(),
        type: "holistic",
        tongueResult,
        fallResult,
        geminiInsight,
      };
      const savedTo = saveResultRecord("holistic", record);

      res.json({ saved_to: savedTo, ...record });
    } catch (err) {
      res.status(500).json({ error: err.message });
    }
  }
);

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`Server TCM + Fall Detection đang chạy tại http://localhost:${PORT}`);
});