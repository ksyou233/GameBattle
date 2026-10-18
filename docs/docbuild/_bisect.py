"""二分定位某个 md 文件里导致 insert_markdown 失败的片段。"""
import json
import pathlib
import subprocess
import sys
import tempfile

SKILL = pathlib.Path(
    r"D:\WorkBuddy\resources\app.asar.unpacked\resources\plugins"
    r"\workbuddy-builtin\skills\tencent-local-office-edit")
PY = sys.executable
EDSDK = SKILL / "edsdk.py"
DOCBUILD = pathlib.Path(r"D:\Learning\设计模式\GameBattle\docs\docbuild")
FID = sys.argv[1]
TARGET = DOCBUILD / sys.argv[2]


def call(text):
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False,
                                     encoding="utf-8", dir=str(DOCBUILD)) as f:
        json.dump({"file_id": FID, "idx": 0, "markdown": text},
                  f, ensure_ascii=False)
        argfile = f.name
    p = subprocess.run([PY, str(EDSDK), "call", "doc_insert_markdown",
                        "--json-file", argfile],
                       capture_output=True, text=True, encoding="utf-8")
    out = (p.stdout or "").strip()
    return "insert_markdown ok" in out, out[:160]


full = TARGET.read_text(encoding="utf-8")
ok, msg = call(full)
print("FULL:", ok, msg)
if ok:
    sys.exit(0)

lines = full.splitlines()
# 按块（以空行分段）二分
blocks = []
cur = []
for ln in lines:
    if ln.strip() == "":
        if cur:
            blocks.append("\n".join(cur))
            cur = []
    else:
        cur.append(ln)
if cur:
    blocks.append("\n".join(cur))
print("blocks:", len(blocks))

for i, b in enumerate(blocks):
    ok, msg = call(b)
    flag = "OK " if ok else "FAIL"
    print(f"[{i:02d}] {flag} :: {b[:70]!r}")
