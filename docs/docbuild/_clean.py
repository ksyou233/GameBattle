import pathlib
import re

BUILD = pathlib.Path(r"D:\Learning\设计模式\GameBattle\docs\docbuild")
pat = re.compile(r"^\s*\*\*（此处插入图[^\n]*\*\*\s*$")
total = 0
for p in sorted(BUILD.glob("c*.md")):
    lines = p.read_text(encoding="utf-8").splitlines()
    keep = [ln for ln in lines if not pat.match(ln)]
    removed = len(lines) - len(keep)
    if removed:
        # 合并因此产生的连续空行
        out = []
        for ln in keep:
            if ln.strip() == "" and out and out[-1].strip() == "":
                continue
            out.append(ln)
        p.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(p.name, "removed", removed)
    total += removed
print("total removed", total)
