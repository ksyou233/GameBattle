"""测试 doc_to_image 云端渲染链路是否可用。"""
import json
import pathlib
import subprocess
import sys
import tempfile

ED = pathlib.Path(
    r"D:\WorkBuddy\resources\app.asar.unpacked\resources\plugins"
    r"\workbuddy-builtin\skills\tencent-local-office-edit\edsdk.py"
)
PY = sys.executable
BUILD = pathlib.Path(__file__).parent


def call(tool, params):
    p = dict(params)
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False,
                                     encoding="utf-8", dir=str(BUILD)) as f:
        json.dump(p, f, ensure_ascii=False)
        af = f.name
    proc = subprocess.run([PY, str(ED), "call", tool, "--json-file", af],
                          capture_output=True, text=True, encoding="utf-8")
    out = (proc.stdout or "").strip()
    try:
        return json.loads(out)
    except Exception:
        return {"_raw": out[:800], "_err": (proc.stderr or "").strip()[:400]}


FID = sys.argv[1]
out_docx = BUILD / "_probe.docx"

print("=== save_file ===")
r = call("save_file", {"file_id": FID, "file_path": str(out_docx)})
print(json.dumps(r, ensure_ascii=False)[:400])
print("exists:", out_docx.exists(),
      out_docx.stat().st_size if out_docx.exists() else 0)

print("\n=== doc_to_image ===")
r = call("doc_to_image", {"file_path": str(out_docx),
                          "output_dir": str(BUILD / "_pages")})
print(json.dumps(r, ensure_ascii=False, indent=2)[:2000])
