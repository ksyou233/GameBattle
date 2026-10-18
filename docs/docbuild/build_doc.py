"""把大作业文档内容灌入 editor_sdk 中的新建文档。

用法：
    python build_doc.py <file_id> <docbuild_dir> <img_dir>

按 manifest 顺序逐条调用 edsdk.py，自动串接 DOC 坐标。
"""
import json
import pathlib
import struct
import subprocess
import sys
import tempfile
import time

SKILL_DIR = pathlib.Path(
    r"D:\WorkBuddy\resources\app.asar.unpacked\resources\plugins"
    r"\workbuddy-builtin\skills\tencent-local-office-edit"
)
PY = sys.executable
EDSDK = SKILL_DIR / "edsdk.py"

FID = sys.argv[1]
BUILD = pathlib.Path(sys.argv[2])
IMG = pathlib.Path(sys.argv[3])


def png_size(path):
    """从 PNG 头里读出宽高。"""
    with open(path, "rb") as f:
        head = f.read(24)
    if head[:8] != b"\x89PNG\r\n\x1a\n":
        return None
    return struct.unpack(">II", head[16:24])


def last_pos():
    resp = call_raw("doc_get_last_operable_pos", {})
    return resp.get("position", 0), resp.get("version", 0)


def settle(min_wait=2.0, max_wait=40.0):
    """等待文档状态稳定。

    图片插入是异步的：图片先是占位结构，稍后才真正落位，期间文档末尾
    坐标会漂移（实测会从 4044 变到 4045）。若在漂移窗口内继续写入，
    编辑器会返回 "HTML segment insertion failed"。
    因此这里轮询末尾坐标与版本号，连续两次不变才认为稳定。
    """
    time.sleep(min_wait)
    waited = min_wait
    prev = None
    while waited < max_wait:
        pos, ver = last_pos()
        cur = (pos, ver)
        if cur == prev:
            return pos
        prev = cur
        time.sleep(1.5)
        waited += 1.5
    return prev[0] if prev else 0


def next_index(resp, idx):
    """取下一步的插入坐标。

    注意：返回值里的 last_edit_index 与 position 并不等价
    （实测 markdown 插入后两者会相差 1），而 position 才是"文档末尾
    可继续追加的位置"。为避免累积偏移，这里统一以
    doc_get_last_operable_pos 的结果为准。
    """
    pos, _ = last_pos()
    if isinstance(pos, int) and pos >= 0:
        return pos
    for key in ("next_index", "position", "last_edit_index"):
        if isinstance(resp.get(key), int) and resp[key] >= 0:
            return resp[key]
    return idx


def call(tool, params, idx):
    params = dict(params)
    params["file_id"] = FID
    params["idx"] = idx
    return call_raw(tool, params)


def call_raw(tool, params):
    params = dict(params)
    params["file_id"] = FID
    with tempfile.NamedTemporaryFile("w", suffix=".json", delete=False,
                                     encoding="utf-8", dir=str(BUILD)) as f:
        json.dump(params, f, ensure_ascii=False)
        argfile = f.name
    proc = subprocess.run([PY, str(EDSDK), "call", tool, "--json-file", argfile],
                          capture_output=True, text=True, encoding="utf-8")
    out = (proc.stdout or "").strip()
    err = (proc.stderr or "").strip()
    if proc.returncode != 0 or not out.startswith("{"):
        raise RuntimeError(f"{tool} 调用失败\nstdout={out[:400]}\nstderr={err[:400]}")
    return json.loads(out)


def md(name):
    return (BUILD / name).read_text(encoding="utf-8")


def cap(text):
    return {"kind": "para", "text": text, "level": 0,
            "text_format": {"font_size": 10.5},
            "paragraph_property": {"jc": "center"}}


def fig(name, caption, width=600):
    return [{"kind": "img", "path": str(IMG / name), "w": width},
            cap(caption)]


def para(text, **kw):
    step = {"kind": "para", "text": text}
    step.update(kw)
    return step


def title(text, size, bold=True, font="黑体", jc="center"):
    return {"kind": "para", "text": text, "level": 0,
            "text_format": {"font_size": size, "bold": bold, "font_family": font},
            "paragraph_property": {"jc": jc}}


def blank(count=1):
    return [{"kind": "para", "text": ""} for _ in range(count)]


# ===========================================================================
#  Manifest
# ===========================================================================
M = []

# ---------------- 封面 ----------------
M += blank(4)
M.append(title("设计模式大作业", 30))
M += blank(2)
M.append(title("游戏战斗与技能系统", 20))
M.append(title("—— 基于 C++17 的八种设计模式综合实现", 14, bold=False, font="宋体"))
M += blank(4)
for line in ["院   系：软件学院",
             "专   业：＿＿＿＿＿＿＿＿＿＿",
             "组   号：＿＿＿＿＿＿＿＿＿＿"]:
    M.append(title(line, 14, bold=False, font="宋体", jc="left"))
M += blank(1)
for _ in range(4):
    M.append(title("姓   名：＿＿＿＿＿＿　班级：＿＿＿＿＿＿　学号：＿＿＿＿＿＿＿＿",
                   14, bold=False, font="宋体", jc="left"))
M += blank(6)
M.append(title("大连理工大学", 18))
M.append(title("Dalian University of Technology", 12, bold=False, font="Times New Roman"))

# ---------------- 分页 + 目录 ----------------
M.append({"kind": "pagebreak"})
M.append(title("目　　录", 16))
M.append({"kind": "toc"})
M.append({"kind": "pagebreak"})

# ---------------- 正文 ----------------
M.append({"kind": "md", "name": "c01-项目介绍.md"})
M.append({"kind": "md", "name": "c02-总体架构.md"})
M += fig("01-主要类图.png", "图 1  系统主要类图")

M.append({"kind": "md", "name": "c03-单例.md"})
M += fig("02-单例模式.png", "图 2  单例模式类图")

M.append({"kind": "md", "name": "c04-抽象工厂.md"})
M += fig("03-抽象工厂模式.png", "图 3  抽象工厂模式类图")

M.append({"kind": "md", "name": "c05-策略.md"})
M += fig("04-策略模式.png", "图 4  策略模式类图")

M.append({"kind": "md", "name": "c06-装饰器.md"})
M += fig("05-装饰器模式.png", "图 5  装饰器模式类图")

M.append({"kind": "md", "name": "c07-观察者.md"})
M += fig("06-观察者模式.png", "图 6  观察者模式类图")

M.append({"kind": "md", "name": "c08-状态.md"})
M += fig("07-状态模式.png", "图 7  状态模式类图")

M.append({"kind": "md", "name": "c09-责任链.md"})
M += fig("08-责任链模式.png", "图 8  责任链模式类图")

M.append({"kind": "md", "name": "c10-组合.md"})
M += fig("09-组合模式.png", "图 9  组合模式类图")

M.append({"kind": "md", "name": "c11-协同与取舍.md"})
M.append({"kind": "md", "name": "c12-关键流程时序.md"})
M += fig("10-时序图-单次攻击结算.png", "图 10  一次攻击的完整结算过程时序图")
M += fig("11-时序图-战斗回合主循环.png", "图 11  回合制战斗主循环时序图")
M += fig("12-时序图-BOSS第二阶段.png", "图 12  BOSS 第二阶段触发链路时序图")

M.append({"kind": "md", "name": "c13-实现.md"})
M.append({"kind": "md", "name": "c14-结果.md"})
M.append({"kind": "md", "name": "c15-心得与分工.md"})


def do_step(step, idx):
    kind = step["kind"]
    if kind == "para":
        params = {"text": step.get("text", "")}
        if step.get("level"):
            params["level"] = step["level"]
        if step.get("text_format"):
            params["text_format"] = step["text_format"]
        if step.get("paragraph_property"):
            params["paragraph_property"] = step["paragraph_property"]
        return call("doc_insert_paragraph_with_text", params, idx)
    if kind == "md":
        return call("doc_insert_markdown", {"markdown": md(step["name"])}, idx)
    if kind == "img":
        w = step.get("w", 600)
        size = png_size(step["path"])
        params = {"image_path": step["path"], "w": w}
        if size:
            ow, oh = size
            h = round(oh * w / ow)
            if h > 780:                 # 太高就按高度反算，避免跨页
                h = 780
                params["w"] = round(ow * h / oh)
            params["h"] = h
        return call("doc_insert_image", params, idx)
    if kind == "toc":
        return call("doc_insert_toc", {"max_level": 3}, idx)
    if kind == "pagebreak":
        return call("doc_insert_page_break", {}, idx)
    raise ValueError("unknown step: " + kind)


# ===========================================================================
#  执行
# ===========================================================================
def main():
    idx = -1
    for step_no, step in enumerate(M, start=1):
        label = step.get("name") or step.get("text") or step.get("path") or step["kind"]
        resp = None
        for attempt in range(1, 9):
            try:
                resp = do_step(step, idx)
                break
            except RuntimeError as exc:
                # editor_sdk 侧是异步处理，连续快速调用偶尔会撞上不一致状态。
                # 这里退避重试，并重新定位文档末尾。
                print(f"    ! 第 {attempt} 次失败，重试：{str(exc)[:80]}")
                time.sleep(min(2.0 * attempt, 8.0))
                idx = settle(2.0, 25.0)
        if resp is None:
            raise RuntimeError(f"步骤 {step_no} 重试 8 次仍失败：{label}")

        if step["kind"] == "img":
            idx = settle(3.0, 45.0)          # 图片落位需要额外等待
        else:
            idx = next_index(resp, idx)
            time.sleep(0.2)
        print(f"[{step_no:02d}/{len(M)}] {step['kind']:10s} idx->{idx:6d}  {str(label)[:44]}")
    print("DONE, final idx =", idx)


if __name__ == "__main__":
    main()
