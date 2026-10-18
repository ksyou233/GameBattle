"""探测云端 DOC→图片 服务的可用性：空白文档 / 纯段落 / 带图片段落。"""
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
OUT = pathlib.Path(r"D:\dmtest")
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
        return {"_raw": out[:300], "_err": (proc.stderr or "").strip()[:300]}


def convert(name):
    p = OUT / name
    r = call("doc_to_image", {"file_path": str(p),
                              "output_dir": str(OUT / (name + "_pages"))})
    ok = isinstance(r, dict) and r.get("page_count")
    print(f"  -> {name}: {'OK pages=' + str(r.get('page_count')) if ok else str(r)[:180]}")
    return ok


def new_doc():
    """create_doc 返回的是纯文本，需要正则抠出 file_id。"""
    import re
    r = call("create_doc", {})
    txt = r.get("_raw", "") if isinstance(r, dict) else str(r)
    m = re.search(r"file_id=(\S+)", txt)
    fid = m.group(1).rstrip(",") if m else None
    print("  file_id:", fid)
    return fid


# --- 1. 空白文档 ---
print("=== 1. 空白文档 ===")
fid = new_doc()
call("save_file", {"file_id": fid, "file_path": str(OUT / "_blank.docx")})
convert("_blank.docx")

# --- 2. 纯段落 ---
print("=== 2. 纯段落 ===")
fid2 = new_doc()
call("doc_insert_paragraph_with_text",
     {"file_id": fid2, "idx": -1, "text": "测试标题", "level": 1})
call("doc_insert_paragraph_with_text",
     {"file_id": fid2, "idx": 5, "text": "这是一段中文正文。"})
call("save_file", {"file_id": fid2, "file_path": str(OUT / "_text.docx")})
convert("_text.docx")

# --- 3. 段落内联图片 ---
print("=== 3. 段落内联图片 ===")
fid3 = new_doc()
img = r"D:\Learning\设计模式\GameBattle\docs\img\02-单例模式.png"
call("doc_batch_execute", {
    "file_id": fid3, "blocks": [
        {"block": "paragraph", "calls": [{
            "tool": "doc_insert_paragraph_with_text",
            "args": {"text": "图前文字"}}]},
        {"block": "paragraph", "calls": [{
            "tool": "doc_insert_paragraph_with_text",
            "args": {"runs": [{"type": "image", "image_path": img,
                               "w": 400, "h": 260}],
                     "paragraph_property": {"jc": "center"}}}]},
        {"block": "paragraph", "calls": [{
            "tool": "doc_insert_paragraph_with_text",
            "args": {"text": "图 1  单例模式类图",
                     "paragraph_property": {"jc": "center"}}}]},
        {"block": "paragraph", "calls": [{
            "tool": "doc_insert_paragraph_with_text",
            "args": {"text": "图后文字"}}]},
    ]})
call("save_file", {"file_id": fid3, "file_path": str(OUT / "_img.docx")})
convert("_img.docx")
