"""排查 doc_insert_markdown 在不同前序状态下是否可插入。"""
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
IMG = pathlib.Path(r"D:\Learning\设计模式\GameBattle\docs\img")
FID = sys.argv[1]

MD = "### 测试标题\n\n这是一段测试正文。\n"


def raw(tool, params):
    params = dict(params)
    params["file_id"] = FID
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False,
                                     encoding="utf-8", dir=str(DOCBUILD)) as f:
        json.dump(params, f, ensure_ascii=False)
        argfile = f.name
    p = subprocess.run([PY, str(EDSDK), "call", tool, "--json-file", argfile],
                       capture_output=True, text=True, encoding="utf-8")
    out = (p.stdout or "").strip()
    return ("ok" in out and out.startswith("{") and "error" not in out[:80]), out[:150]


def last_pos():
    ok, out = raw("doc_get_last_operable_pos", {})
    return json.loads(out)["position"]


def try_step(label, tool, params, idx=None):
    if idx is None:
        idx = last_pos()
    ok, out = raw(tool, dict(params, idx=idx))
    print(f"{label:34s} idx={idx:<7} ok={ok}  {out[:90]}")
    return ok


print("== A. 空文档直接插 markdown ==")
try_step("md@empty", "doc_insert_markdown", {"markdown": MD})

print("== B. 插一段普通段落后插 markdown ==")
try_step("para", "doc_insert_paragraph_with_text", {"text": "普通段落"})
try_step("md@after-para", "doc_insert_markdown", {"markdown": MD})

print("== C. 插图片后插 markdown ==")
try_step("image", "doc_insert_image",
         {"image_path": str(IMG / "02-单例模式.png"), "w": 600})
try_step("md@after-image", "doc_insert_markdown", {"markdown": MD})

print("== D. 图片后先插空段落再插 markdown ==")
try_step("blank-para", "doc_insert_paragraph_with_text", {"text": ""})
try_step("md@after-blank", "doc_insert_markdown", {"markdown": MD})

print("== E. 分页符后插 markdown ==")
try_step("pagebreak", "doc_insert_page_break", {})
try_step("md@after-pagebreak", "doc_insert_markdown", {"markdown": MD})
