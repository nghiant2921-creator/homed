import { execFile } from "child_process";
import { promisify } from "util";
import path from "path";
import fs from "fs";

const execFileAsync = promisify(execFile);

function getPythonExecutable() {
    const winPython = path.resolve(".venv", "Scripts", "python.exe");
    const unixPython = path.resolve(".venv", "bin", "python");
    if (fs.existsSync(winPython)) return winPython;
    if (fs.existsSync(unixPython)) return unixPython;
    return "python";
}

export async function runPythonScript(scriptRelativePath, mediaFilePath) {
    const pythonBin = getPythonExecutable();
    const scriptPath = path.resolve(scriptRelativePath);
    const absMediaPath = path.resolve(mediaFilePath);

    const { stdout, stderr } = await execFileAsync(
        pythonBin,
        [scriptPath, absMediaPath],
        {
            timeout: 120000, // Tối đa 2 phút cho máy CPU văn phòng
            maxBuffer: 10 * 1024 * 1024,
            env: { ...process.env, PYTHONIOENCODING: "utf-8" },
        }
    );

    // Lọc dòng JSON cuối cùng phòng trường hợp thư viện in cảnh báo ra stdout
    const lines = stdout.trim().split("\n");
    for (let i = lines.length - 1; i >= 0; i--) {
        if (lines[i].trim().startsWith("{")) {
            return JSON.parse(lines.slice(i).join("\n"));
        }
    }

    throw new Error(`Không đọc được JSON từ ${scriptRelativePath}. Stderr: ${stderr}`);
}