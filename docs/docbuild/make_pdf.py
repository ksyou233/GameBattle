"""把 docbuild 下的章节 Markdown + UML 图片渲染为一份 A4 版式的 PDF。

设计要点
--------
* 字体：正文宋体（simsun）、标题黑体（simhei）、斜体楷体（simkai）。
  ReportLab 不做字体回退，而 ``<b>`` 需要真实存在一个"加粗"字体，
  所以这里把黑体注册成宋体的 bold 变体 —— 中文学术文档的常见做法。
* 目录：使用 ``TableOfContents`` + ``multiBuild``，页码由 ReportLab
  在第二遍排版时回填，因此是真实页码而非占位。
* 代码块：等宽感不强但能覆盖中文注释，统一用宋体小字号 + 浅灰底框；
  过长的行按"半角等宽"折算后硬折行，避免溢出页面。
"""
import pathlib
import re
import sys
import xml.sax.saxutils as sax

from reportlab.lib import colors
from reportlab.lib.enums import TA_CENTER, TA_JUSTIFY, TA_LEFT, TA_RIGHT
from reportlab.lib.pagesizes import A4
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import cm
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, Frame, Image, KeepTogether,
                                NextPageTemplate, PageBreak, PageTemplate,
                                Paragraph, Spacer, Table, TableStyle)
from reportlab.platypus.tableofcontents import TableOfContents

BUILD = pathlib.Path(__file__).parent
IMG = BUILD.parent / "img"
OUT = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else BUILD / "out.pdf"

FONTS = pathlib.Path("C:/Windows/Fonts")

# ---------------------------------------------------------------- 字体注册
pdfmetrics.registerFont(TTFont("Song", str(FONTS / "simsun.ttc"), subfontIndex=0))
pdfmetrics.registerFont(TTFont("Hei", str(FONTS / "simhei.ttf")))
pdfmetrics.registerFont(TTFont("Kai", str(FONTS / "simkai.ttf")))
# 宋体/黑体都没有 → ⇄ ▶ ⁿ ⁴ ↳ ⇢ ⚠ 这类符号，用 Segoe UI Symbol 兜底
pdfmetrics.registerFont(TTFont("Sym", str(FONTS / "seguisym.ttf")))
pdfmetrics.registerFontFamily("Song", normal="Song", bold="Hei",
                              italic="Kai", boldItalic="Hei")
pdfmetrics.registerFontFamily("Hei", normal="Hei", bold="Hei",
                              italic="Hei", boldItalic="Hei")
pdfmetrics.registerFontFamily("Kai", normal="Kai", bold="Hei",
                              italic="Kai", boldItalic="Hei")

_SONG_G = pdfmetrics.getFont("Song").face.charToGlyph
_HEI_G = pdfmetrics.getFont("Hei").face.charToGlyph
# 只要宋体或黑体任一缺字，就改用符号字体
NEED_SYM = {chr(o) for o in range(0x2000, 0x10000)
            if o not in _SONG_G or o not in _HEI_G}


def fix_glyphs(s):
    """把两端字体都缺的字符用 <font name="Sym"> 包起来。

    ReportLab 不做字体回退，缺字会直接渲染成空白；这里做逐字符兜底，
    既保住了程序真实输出里的 ▶ / ↳，也让 "2ⁿ" 的上标正确显示。
    """
    if not any(c in NEED_SYM for c in s):
        return s
    out, inside = [], False
    for ch in s:
        need = ch in NEED_SYM
        if need != inside:
            out.append('<font name="Sym">' if need else "</font>")
            inside = need
        out.append(ch)
    if inside:
        out.append("</font>")
    return "".join(out)

PAGE_W, PAGE_H = A4
LAND_W, LAND_H = A4[1], A4[0]
MARGIN_L = MARGIN_R = 2.4 * cm
MARGIN_T = 2.4 * cm
MARGIN_B = 2.2 * cm
USABLE = PAGE_W - MARGIN_L - MARGIN_R

# ---------------------------------------------------------------- 样式
SS = getSampleStyleSheet()

S = {
    "cover_title": ParagraphStyle("cover_title", fontName="Hei", fontSize=30,
                                  leading=42, alignment=TA_CENTER),
    "cover_sub": ParagraphStyle("cover_sub", fontName="Song", fontSize=16,
                                leading=26, alignment=TA_CENTER),
    "cover_note": ParagraphStyle("cover_note", fontName="Song", fontSize=13,
                                 leading=24, alignment=TA_CENTER),
    "cover_info": ParagraphStyle("cover_info", fontName="Song", fontSize=12.5,
                                 leading=26, alignment=TA_LEFT,
                                 leftIndent=2.6 * cm),
    "cover_name": ParagraphStyle("cover_name", fontName="Song", fontSize=11.5,
                                 leading=26, alignment=TA_LEFT,
                                 leftIndent=1.8 * cm),
    "cover_school": ParagraphStyle("cover_school", fontName="Hei", fontSize=18,
                                   leading=26, alignment=TA_CENTER),
    "h2": ParagraphStyle("H2", fontName="Hei", fontSize=16, leading=24,
                         spaceBefore=14, spaceAfter=9, keepWithNext=1),
    "h3": ParagraphStyle("H3", fontName="Hei", fontSize=13.5, leading=21,
                         spaceBefore=11, spaceAfter=6.5, keepWithNext=1),
    "h4": ParagraphStyle("H4", fontName="Hei", fontSize=11.8, leading=19,
                         spaceBefore=8.5, spaceAfter=5, keepWithNext=1),
    "body": ParagraphStyle("Body", fontName="Song", fontSize=10.5, leading=17.5,
                           alignment=TA_JUSTIFY, firstLineIndent=21,
                           spaceAfter=5.5, wordWrap="CJK"),
    "plain": ParagraphStyle("Plain", fontName="Song", fontSize=10.5,
                            leading=17.5, alignment=TA_JUSTIFY, spaceAfter=5.5, wordWrap="CJK"),
    "list": ParagraphStyle("List", fontName="Song", fontSize=10.5, leading=17.5,
                           alignment=TA_JUSTIFY, leftIndent=21,
                           bulletIndent=7, spaceAfter=3.5, wordWrap="CJK"),
    "quote": ParagraphStyle("Quote", fontName="Kai", fontSize=10.5, leading=17,
                            alignment=TA_JUSTIFY, leftIndent=21, rightIndent=14,
                            textColor=colors.HexColor("#333333"), wordWrap="CJK"),
    "code": ParagraphStyle("Code", fontName="Song", fontSize=8, leading=11.6,
                           textColor=colors.HexColor("#1A1A1A")),
    "caption": ParagraphStyle("Caption", fontName="Song", fontSize=10,
                              leading=15, alignment=TA_CENTER,
                              spaceBefore=5, spaceAfter=14),
    "cell": ParagraphStyle("Cell", fontName="Song", fontSize=8.6, leading=12.6, wordWrap="CJK"),
    "cellh": ParagraphStyle("CellH", fontName="Hei", fontSize=8.8, leading=12.8, wordWrap="CJK"),
    "toc_h": ParagraphStyle("TOCHead", fontName="Hei", fontSize=16, leading=26,
                            alignment=TA_CENTER, spaceAfter=18),
    "toc0": ParagraphStyle("TOC0", fontName="Hei", fontSize=11, leading=20,
                           spaceBefore=5),
    "toc1": ParagraphStyle("TOC1", fontName="Song", fontSize=10.5, leading=18,
                           leftIndent=16),
    "toc2": ParagraphStyle("TOC2", fontName="Song", fontSize=10, leading=17,
                           leftIndent=32, textColor=colors.HexColor("#333333")),
}

# ---------------------------------------------------------------- 行内解析
def inline(text):
    """Markdown 行内语法 -> ReportLab 迷你 HTML。"""
    out = sax.escape(text, {'"': "&quot;"})
    out = re.sub(r"`([^`]+)`",
                 r'<font name="Song" color="#A63A5B">\1</font>', out)
    out = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", out)
    out = re.sub(r"(?<!\*)\*([^*\n]+)\*(?!\*)", r"<i>\1</i>", out)
    return fix_glyphs(out)


def disp_width(s):
    """按半角折算的显示宽度（中文算 2）。"""
    return sum(2 if ord(c) > 0x2E80 else 1 for c in s)


def wrap_code(line, limit):
    """把过长的代码行硬折行，续行缩进 4 空格。"""
    if disp_width(line) <= limit:
        return [line]
    parts, cur, w = [], "", 0
    for ch in line:
        cw = 2 if ord(ch) > 0x2E80 else 1
        if w + cw > limit and cur:
            parts.append(cur)
            cur, w = "    ", 4
        cur += ch
        w += cw
    if cur.strip():
        parts.append(cur)
    return parts


# ---------------------------------------------------------------- 块级解析
BLOCK_START = re.compile(r"^(#{1,6}\s|```|\||>\s|\s*([-*]|\d+\.)\s)")


def parse(text):
    """把一段 Markdown 拆成 ('p'|'h'|'code'|'table'|'list'|'quote', ...) 序列。"""
    lines = text.split("\n")
    out, i, n = [], 0, len(lines)
    while i < n:
        ln = lines[i]
        if ln.startswith("```"):
            i += 1
            buf = []
            while i < n and not lines[i].startswith("```"):
                buf.append(lines[i])
                i += 1
            i += 1
            out.append(("code", "\n".join(buf)))
            continue
        if ln.lstrip().startswith("|"):
            rows = []
            while i < n and lines[i].lstrip().startswith("|"):
                rows.append(lines[i].strip())
                i += 1
            out.append(("table", rows))
            continue
        m = re.match(r"^(#{1,6})\s+(.*)$", ln)
        if m:
            out.append(("h", len(m.group(1)), m.group(2).strip()))
            i += 1
            continue
        if ln.lstrip().startswith(">"):
            buf = []
            while i < n and lines[i].lstrip().startswith(">"):
                buf.append(lines[i].lstrip()[1:].strip())
                i += 1
            out.append(("quote", " ".join(buf)))
            continue
        if re.match(r"^\s*([-*]|\d+\.)\s+", ln):
            ordered = bool(re.match(r"^\s*\d+\.\s+", ln))
            items, start = [], None
            while i < n and re.match(r"^\s*([-*]|\d+\.)\s+", lines[i]):
                mm = re.match(r"^\s*(\d+)\.\s+", lines[i])
                if mm and start is None:
                    start = int(mm.group(1))
                items.append(re.sub(r"^\s*([-*]|\d+\.)\s+", "", lines[i]).strip())
                i += 1
            out.append(("list", ordered, items, start))
            continue
        if not ln.strip():
            i += 1
            continue
        buf = [ln.strip()]
        i += 1
        while (i < n and lines[i].strip()
               and not BLOCK_START.match(lines[i])):
            buf.append(lines[i].strip())
            i += 1
        out.append(("p", " ".join(buf)))
    return out


# ---------------------------------------------------------------- 表格
def split_row(row):
    return [c.strip() for c in row.strip().strip("|").split("|")]


def is_sep(row):
    cells = split_row(row)
    return bool(cells) and all(re.fullmatch(r":?-{2,}:?", c) for c in cells)


def make_table(rows):
    header = split_row(rows[0])
    body = [split_row(r) for r in rows[1:] if not is_sep(r)]
    ncol = max([len(header)] + [len(r) for r in body] or [1])

    def pad(r):
        return list(r) + [""] * (ncol - len(r))

    header, body = pad(header), [pad(r) for r in body]

    # 按内容宽度分配列宽：先按自然宽度分配，再施加下限/上限，
    # 避免"某一列内容很长就把别的列挤成一字一行"。
    natural = []
    for c in range(ncol):
        w = max([disp_width(header[c])] + [disp_width(r[c]) for r in body] or [1])
        natural.append(max(w, 4))
    total = sum(natural)
    floor = min(0.115 * USABLE, USABLE / (ncol * 1.6))
    cap = 0.45 * USABLE
    col_w = [min(cap, max(floor, USABLE * w / total)) for w in natural]
    s = sum(col_w)
    if s > USABLE:                       # 超宽：只压缩高于下限的列
        over = s - USABLE
        slack = [max(0.0, w - floor) for w in col_w]
        ss = sum(slack) or 1.0
        col_w = [w - over * k / ss for w, k in zip(col_w, slack)]
    elif s < USABLE:                     # 未满：按比例补足，铺满版心
        col_w = [w + (USABLE - s) * w / s for w in col_w]

    data = [[Paragraph(inline(c), S["cellh"]) for c in header]]
    for r in body:
        data.append([Paragraph(inline(c), S["cell"]) for c in r])

    t = Table(data, colWidths=col_w, repeatRows=1, hAlign="CENTER")
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#E8EDF3")),
        ("LINEBELOW", (0, 0), (-1, 0), 0.9, colors.HexColor("#7A8CA0")),
        ("GRID", (0, 0), (-1, -1), 0.35, colors.HexColor("#B8C2CC")),
        ("VALIGN", (0, 0), (-1, -1), "MIDDLE"),
        ("LEFTPADDING", (0, 0), (-1, -1), 4),
        ("RIGHTPADDING", (0, 0), (-1, -1), 4),
        ("TOPPADDING", (0, 0), (-1, -1), 3.5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 3.5),
    ]))
    return t


# ---------------------------------------------------------------- 代码块
CODE_LIMIT = 106
# 单个代码框的最大行数：整块高度必须小于一页版心（约 699pt），
# 46 行 × 11.6pt ≈ 534pt，即使全部折行也仍在版心之内。
CODE_MAX_LINES = 46


def make_code(src):
    lines = []
    for raw in src.split("\n"):
        lines.extend(wrap_code(raw, CODE_LIMIT))
    # 顺序很重要：先把空白替换成实体，再补字体标签，
    # 否则 fix_glyphs 插进来的标签内部空格会被一起替换掉。
    txt = sax.escape("\n".join(lines))
    txt = txt.replace("\n", "<br/>").replace(" ", "&nbsp;")
    para = Paragraph(fix_glyphs(txt), S["code"])
    box = Table([[para]], colWidths=[USABLE])
    box.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), colors.HexColor("#F5F5F2")),
        ("BOX", (0, 0), (-1, -1), 0.45, colors.HexColor("#C6C6BE")),
        ("LEFTPADDING", (0, 0), (-1, -1), 7),
        ("RIGHTPADDING", (0, 0), (-1, -1), 7),
        ("TOPPADDING", (0, 0), (-1, -1), 5),
        ("BOTTOMPADDING", (0, 0), (-1, -1), 5),
    ]))
    return box


def make_code_blocks(src):
    """长代码块自动切分成若干个"一页装得下"的框，避免 LayoutError。"""
    lines = []
    for raw in src.split("\n"):
        lines.extend(wrap_code(raw, CODE_LIMIT))
    if len(lines) <= CODE_MAX_LINES:
        return [make_code(src)]
    return [make_code("\n".join(lines[i:i + CODE_MAX_LINES]))
            for i in range(0, len(lines), CODE_MAX_LINES)]


# ---------------------------------------------------------------- 图片
def _fit(path, max_w, max_h):
    """按给定外框等比缩放，返回 (w, h, path)。"""
    from PIL import Image as PILImage
    with PILImage.open(path) as im:
        iw, ih = im.size
    w, h = max_w, ih * max_w / iw
    if h > max_h:
        h, w = max_h, iw * max_h / ih
    return w, h


def make_fig(name, caption, land=False):
    """图 + 图题。land=True 时铺满横向页。"""
    path = IMG / name
    if land:
        fw = LAND_W - MARGIN_L - MARGIN_R
        # 留位给 Spacer + 图题（约 43pt），否则图题会被挤到下一页
        fh = LAND_H - MARGIN_T - MARGIN_B - 48
        w, h = _fit(path, fw, fh)
    else:
        w, h = _fit(path, USABLE, 620)
    img = Image(str(path), width=w, height=h)
    img.hAlign = "CENTER"
    return KeepTogether([Spacer(1, 9), img,
                         Paragraph(inline(caption), S["caption"])])


# ---------------------------------------------------------------- 流构建
def blocks_to_flow(blocks, state=None):
    """把解析结果转成 ReportLab 流对象。

    只有"编号章节"（形如 ``## 3 实现``）才另起一页；第一章不另起，
    这样"大作业类型"声明与"1 项目介绍"共用第一页，不会留下大片空白。
    state 里的 seen_chapter 跨文件累积，用于识别"第一章"。
    """
    if state is None:
        state = {"seen_chapter": False}
    flow = []
    for b in blocks:
        kind = b[0]
        if kind == "h":
            lvl, text = b[1], b[2]
            if lvl <= 2:
                numbered = bool(re.match(r"^\d+(\.\d+)*[\s、]", text)) or \
                    text.startswith("附录")
                st = ParagraphStyle("H2", parent=S["h2"])
                if numbered:
                    if state["seen_chapter"]:
                        st.pageBreakBefore = 1
                    state["seen_chapter"] = True
                flow.append(Paragraph(inline(text), st))
            elif lvl == 3:
                flow.append(Paragraph(inline(text), S["h3"]))
            else:
                flow.append(Paragraph(inline(text), S["h4"]))
        elif kind == "p":
            flow.append(Paragraph(inline(b[1]), S["body"]))
        elif kind == "quote":
            flow.append(Paragraph(inline(b[1]), S["quote"]))
        elif kind == "list":
            _, ordered, items, start = b
            for k, it in enumerate(items):
                mark = f"{start + k}." if (ordered and start) else (
                    f"{k + 1}." if ordered else "•")
                flow.append(Paragraph(inline(it), S["list"],
                                      bulletText=mark))
        elif kind == "code":
            flow.append(Spacer(1, 3))
            flow.extend(make_code_blocks(b[1]))
            flow.append(Spacer(1, 8))
        elif kind == "table":
            flow.append(Spacer(1, 3))
            flow.append(make_table(b[1]))
            flow.append(Spacer(1, 9))
    return flow


# ---------------------------------------------------------------- 文档模板
class DocTpl(BaseDocTemplate):
    def __init__(self, filename, **kw):
        BaseDocTemplate.__init__(self, filename, **kw)
        frame_cover = Frame(MARGIN_L, MARGIN_B, USABLE,
                            PAGE_H - MARGIN_T - MARGIN_B, id="cover")
        frame_body = Frame(MARGIN_L, MARGIN_B, USABLE,
                           PAGE_H - MARGIN_T - MARGIN_B, id="body")
        frame_land = Frame(MARGIN_L, MARGIN_B, LAND_W - MARGIN_L - MARGIN_R,
                           LAND_H - MARGIN_T - MARGIN_B, id="land")
        self.addPageTemplates([
            PageTemplate(id="Cover", frames=[frame_cover],
                         onPage=self._cover_page),
            PageTemplate(id="Body", frames=[frame_body],
                         onPage=self._body_page),
            # 横向页：专供"图 1 主要类图"这类大图，避免缩到看不清
            PageTemplate(id="Land", frames=[frame_land],
                         onPage=self._land_page,
                         pagesize=(LAND_W, LAND_H)),
        ])

    def _cover_page(self, canv, doc):
        pass

    def _footer(self, canv, width):
        canv.saveState()
        canv.setFont("Song", 9)
        canv.setFillColor(colors.HexColor("#555555"))
        canv.drawCentredString(width / 2, MARGIN_B / 2 + 4,
                               f"— {canv.getPageNumber()} —")
        canv.setStrokeColor(colors.HexColor("#BBBBBB"))
        canv.setLineWidth(0.4)
        canv.line(MARGIN_L, MARGIN_B / 2 + 16, width - MARGIN_R,
                  MARGIN_B / 2 + 16)
        canv.restoreState()

    def _body_page(self, canv, doc):
        self._footer(canv, PAGE_W)

    def _land_page(self, canv, doc):
        self._footer(canv, LAND_W)

    def afterFlowable(self, flowable):
        if not isinstance(flowable, Paragraph):
            return
        name = flowable.style.name
        # 目录只收录"章"（H2）与"节"（H3）两级；四级标题不入选，
        # 否则目录会长达三页，且细目在答辩时几乎不会被检索。
        if name in ("H2", "H3"):
            lvl = {"H2": 0, "H3": 1}[name]
            self.notify("TOCEntry", (lvl, flowable.getPlainText(),
                                     self.page))


# ---------------------------------------------------------------- 封面
BLANKS = {"n": 0}


def cover_flow():
    def sp(h):
        return Spacer(1, h)
    f = [sp(2.1 * cm)]
    f.append(Paragraph("设计模式大作业", S["cover_title"]))
    f.append(sp(1.5 * cm))
    f.append(Paragraph("游戏战斗与技能系统", S["cover_sub"]))
    f.append(sp(0.28 * cm))
    f.append(Paragraph("—— 基于 C++17 的八种设计模式综合实现",
                       S["cover_note"]))
    f.append(sp(2.2 * cm))
    for line in ["院　　系：软件学院",
                 "专　　业：＿＿＿＿＿＿＿＿＿＿",
                 "组　　号：＿＿＿＿＿＿＿＿＿＿"]:
        f.append(Paragraph(line, S["cover_info"]))
    f.append(sp(0.35 * cm))
    for _ in range(4):
        f.append(Paragraph("姓　　名：＿＿＿＿＿＿　班级：＿＿＿＿　"
                           "学号：＿＿＿＿＿＿", S["cover_name"]))
    f.append(sp(2.4 * cm))
    f.append(Paragraph("大连理工大学", S["cover_school"]))
    f.append(Paragraph("Dalian University of Technology", S["cover_note"]))
    return f


# ---------------------------------------------------------------- 主流程
MANIFEST = [
    ("md", "c01-项目介绍.md"),
    ("md", "c02-总体架构.md"),
    ("figland", "01-主要类图.png", "图 1　系统主要类图"),
    ("md", "c03-单例.md"),
    ("fig", "02-单例模式.png", "图 2　单例模式类图"),
    ("md", "c04-抽象工厂.md"),
    ("fig", "03-抽象工厂模式.png", "图 3　抽象工厂模式类图"),
    ("md", "c05-策略.md"),
    ("fig", "04-策略模式.png", "图 4　策略模式类图"),
    ("md", "c06-装饰器.md"),
    ("fig", "05-装饰器模式.png", "图 5　装饰器模式类图"),
    ("md", "c07-观察者.md"),
    ("fig", "06-观察者模式.png", "图 6　观察者模式类图"),
    ("md", "c08-状态.md"),
    ("fig", "07-状态模式.png", "图 7　状态模式类图"),
    ("md", "c09-责任链.md"),
    ("fig", "08-责任链模式.png", "图 8　责任链模式类图"),
    ("md", "c10-组合.md"),
    ("fig", "09-组合模式.png", "图 9　组合模式类图"),
    ("md", "c11-协同与取舍.md"),
    ("md", "c12-关键流程时序.md"),
    ("fig", "10-时序图-单次攻击结算.png", "图 10　一次攻击的完整结算过程时序图"),
    ("fig", "11-时序图-战斗回合主循环.png", "图 11　回合制战斗主循环时序图"),
    ("fig", "12-时序图-BOSS第二阶段.png", "图 12　BOSS 第二阶段触发链路时序图"),
    ("md", "c13-实现.md"),
    ("md", "c14-结果.md"),
    ("md", "c15-心得与分工.md"),
    ("md", "c16-参考文献.md"),
]


def build():
    doc = DocTpl(str(OUT), pagesize=A4,
                 leftMargin=MARGIN_L, rightMargin=MARGIN_R,
                 topMargin=MARGIN_T, bottomMargin=MARGIN_B,
                 title="游戏战斗与技能系统——基于 C++17 的八种设计模式综合实现",
                 author="设计模式课程大作业")
    toc = TableOfContents()
    toc.levelStyles = [S["toc0"], S["toc1"], S["toc2"]]
    toc.dotsMinLevel = 0

    story = list(cover_flow())
    story.append(NextPageTemplate("Body"))
    story.append(PageBreak())
    story.append(Paragraph("目　　录", S["toc_h"]))
    story.append(toc)
    story.append(PageBreak())

    state = {"seen_chapter": False}
    for item in MANIFEST:
        if item[0] == "md":
            text = (BUILD / item[1]).read_text(encoding="utf-8")
            story.extend(blocks_to_flow(parse(text), state))
        elif item[0] == "figland":
            story.append(NextPageTemplate("Land"))
            story.append(PageBreak())
            story.append(make_fig(item[1], item[2], land=True))
            story.append(NextPageTemplate("Body"))
            story.append(PageBreak())
        else:
            story.append(make_fig(item[1], item[2]))

    doc.multiBuild(story)
    return doc


if __name__ == "__main__":
    d = build()
    print("OK ->", OUT, "pages =", d.page)
