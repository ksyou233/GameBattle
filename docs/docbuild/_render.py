"""把 PDF 的指定页渲染成 PNG，便于逐页目视检查。"""
import pathlib
import sys

import pypdfium2 as pdfium

src = sys.argv[1]
pages = [int(x) for x in sys.argv[2].split(",")]
outdir = pathlib.Path(sys.argv[3])
outdir.mkdir(parents=True, exist_ok=True)

pdf = pdfium.PdfDocument(src)
print("总页数:", len(pdf))
for p in pages:
    if p < 1 or p > len(pdf):
        print(f"  跳过 p{p}（越界）")
        continue
    page = pdf[p - 1]
    img = page.render(scale=1.35).to_pil()
    dst = outdir / f"p{p:02d}.png"
    img.save(dst)
    print(f"  p{p} -> {dst}  {img.size}")
