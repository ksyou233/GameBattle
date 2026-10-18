"""逐段探测 doc_insert_markdown 能接受哪些语法。"""
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
FID = sys.argv[1]
TMP = pathlib.Path(r"D:\Learning\设计模式\GameBattle\docs\docbuild")


def call(tool, params):
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False,
                                     encoding="utf-8", dir=str(TMP)) as f:
        json.dump(params, f, ensure_ascii=False)
        argfile = f.name
    p = subprocess.run([PY, str(EDSDK), "call", tool, "--json-file", argfile],
                       capture_output=True, text=True, encoding="utf-8")
    return p.returncode, (p.stdout or "").strip()


CASES = [
    ("h2+para", "## 标题二\n\n这是一段正文。\n"),
    ("h3+para", "### 标题三\n\n这是一段正文。\n"),
    ("bold", "**加粗文字** 和普通文字\n"),
    ("table2", "| A | B |\n| --- | --- |\n| 1 | 2 |\n"),
    ("table4", "| 变化维度 | 表现 | 后果 | 备注 |\n| --- | --- | --- | --- |\n| 一 | 二 | 三 | 四 |\n"),
    ("superscript", "增长 2ⁿ 倍\n"),
    ("code_fence", "```cpp\nint a = 1;\n```\n"),
    ("ordered", "1. 第一\n2. 第二\n"),
    ("blockquote", "> 引用文字\n"),
    ("hr", "---\n"),
    ("link", "[文字](https://example.com)\n"),
]

idx = 0
for name, text in CASES:
    rc, out = call("doc_insert_markdown",
                   {"file_id": FID, "idx": idx, "markdown": text})
    ok = out.startswith("{") and '"ok": true' in out
    if ok:
        try:
            idx = json.loads(out).get("position", idx)
        except Exception:
            pass
    msg = out[:100].replace("\n", " ")
    print(f"{name:12s} ok={ok} idx={idx}  {msg}")
