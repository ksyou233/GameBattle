"""探查当前文档真实状态：末尾坐标、大纲、图片数。"""
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
FID = sys.argv[1] if len(sys.argv) > 1 else "new_doc_4239155002200_cec7"
BUILD = pathlib.Path(__file__).parent


def call(tool, params):
    p = dict(params)
    p["file_id"] = FID
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
        return {"_raw": out, "_err": (proc.stderr or "").strip()}


print("=== last pos ===")
r = call("doc_get_last_operable_pos", {})
print(json.dumps(r, ensure_ascii=False, indent=2)[:600])

print("\n=== images ===")
r = call("doc_get_images", {})
imgs = r.get("images") or r.get("data") or r
print(json.dumps(imgs, ensure_ascii=False)[:600] if not isinstance(imgs, list)
      else f"共 {len(imgs)} 张")

print("\n=== outline ===")
r = call("doc_get_outline", {})
print(json.dumps(r, ensure_ascii=False, indent=2)[:1500])
