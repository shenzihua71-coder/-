# -*- coding: utf-8 -*-
"""将《51单片机智能门锁代码精讲.md》转换为 Word 文档（分章节、带目录）"""
import re
from docx import Document
from docx.shared import Pt, Cm, RGBColor
from docx.enum.text import WD_ALIGN_PARAGRAPH
from docx.enum.table import WD_TABLE_ALIGNMENT
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

SRC = r"C:\Users\32375\Desktop\51单片机智能门锁代码精讲.md"
DST = r"C:\Users\32375\Desktop\51单片机智能门锁代码精讲.docx"

BODY_EAST = "微软雅黑"
BODY_ASCII = "Calibri"
CODE_EAST = "微软雅黑"
CODE_ASCII = "Consolas"

HEADING_SIZES = {22: 0, 16: 1, 13: 2, 11.5: 3}


# ---------------- 低层样式辅助 ----------------
def set_font(run, east, ascii_font, size, bold=False, color=None, italic=False):
    run.font.name = ascii_font
    run.font.size = Pt(size)
    run.font.bold = bold
    run.font.italic = italic
    if color:
        run.font.color.rgb = RGBColor(*color)
    rpr = run._element.get_or_add_rPr()
    rf = rpr.find(qn('w:rFonts'))
    if rf is None:
        rf = OxmlElement('w:rFonts')
        rpr.append(rf)
    rf.set(qn('w:ascii'), ascii_font)
    rf.set(qn('w:hAnsi'), ascii_font)
    rf.set(qn('w:eastAsia'), east)


def add_runs(paragraph, text, base_size=10.5, bold=False, italic=False,
             code=False, color=None):
    run = paragraph.add_run(text)
    if code:
        set_font(run, CODE_EAST, CODE_ASCII, base_size, bold=False,
                 color=(0xC0, 0x39, 0x2B))
    else:
        set_font(run, BODY_EAST, BODY_ASCII, base_size, bold=bold,
                 italic=italic, color=color)


def add_inline(paragraph, text, base_size=10.5, bold=False, color=None):
    """把带 `code` / **bold** / *ital* 的 markdown 行内标记渲染成格式化 runs"""
    def _add(seg, is_bold, is_code, is_ital=False):
        add_runs(paragraph, seg, base_size,
                 bold=bold or is_bold, italic=is_ital,
                 code=is_code, color=color)

    # 转义符保护
    placeholders = {}

    def _save(m):
        key = "\x00%d\x00" % len(placeholders)
        placeholders[key] = m.group(1)
        return key

    text = re.sub(r'\\([\\`*_{}\[\]()#+\-.!|])', _save, text)
    text = text.replace('`', '')
    text = re.sub(r'(?<!\*)\*\*(?!\*)', '', text)

    # 1) 空格 → tab（表格单元格里的普通分隔符还原为空格）
    parts = []
    for span in re.split(r'(\s+)', text):
        if not span:
            continue
        if span.isspace():
            in_cell = "\x00" in span
            if "\t" in text and not in_cell:
                parts.append(("\t" if len(span) > 1 else " ", False, False))
            else:
                parts.append((span, False, False))
        else:
            rendered = _render_marked(span)
            if len(rendered) > 1:
                parts.append(("__LIST__", rendered))
            else:
                parts.append(rendered[0] if rendered else ("", False, False))

    # 2) 合并相邻纯文本并清掉孤立的 ** 残留
    merged = []
    for item in parts:
        if not item:
            continue
        if item[0] == "__LIST__":
            merged.append(item)
            continue
        seg, is_bold, is_code = item
        if not seg:
            continue
        if (not is_bold and not is_code and seg.isspace()
                and merged and merged[-1][0] != "__LIST__"
                and not merged[-1][1] and not merged[-1][2]
                and merged[-1][0].isspace()):
            continue
        if (not is_bold and not is_code
                and merged and merged[-1][0] != "__LIST__"
                and not merged[-1][1] and not merged[-1][2]):
            prev = merged[-1][0]
            if (prev and not prev[-1].isspace() and seg and not seg[0].isspace()
                    and not seg.startswith('_')):
                merged[-1] = (prev + seg, False, False)
                continue
        merged.append((seg, is_bold, is_code))

    # 3) 输出 runs，去掉孤立 **
    for item in merged:
        if item[0] == "__LIST__":
            for seg, is_bold, is_code in item[1]:
                _add(seg, is_bold, is_code)
            continue
        seg, is_bold, is_code = item
        if not is_bold and not is_code:
            seg = re.sub(r'\*\*', '', seg)
            if not seg:
                continue
        _add(seg, is_bold, is_code)

    # 还原转义字符
    if placeholders:
        for run in paragraph.runs:
            for k, v in placeholders.items():
                if k in run.text:
                    run.text = run.text.replace(k, v)


def _render_marked(text):
    """在不含空格的 span 内处理 inline 标记，返回 (seg, bold, code) 列表"""
    pattern = re.compile(
        r'(?P<code>`[^`]+`)'
        r'|(?P<bold>\*\*[^*]+\*\*|\*\*[^*]+)'
        r'|(?P<ital>\*[^*]+\*)'
    )
    out = []
    pos = 0
    for m in pattern.finditer(text):
        if m.start() > pos:
            seg = text[pos:m.start()]
            if seg:
                out.append((seg, False, False))
        if m.group('code'):
            out.append((m.group('code')[1:-1], False, True))
        elif m.group('bold'):
            b = m.group('bold')
            if b.startswith('**'):
                b = b[2:]
            if b.endswith('**'):
                b = b[:-2]
            out.append((b, True, False))
        elif m.group('ital'):
            out.append((m.group('ital')[1:-1], False, False))
        pos = m.end()
    if pos < len(text):
        seg = text[pos:]
        if seg:
            out.append((seg, False, False))
    return out


def style_para(paragraph, before=0, after=6, line=1.35, align=None):
    pf = paragraph.paragraph_format
    pf.space_before = Pt(before)
    pf.space_after = Pt(after)
    pf.line_spacing = line
    if align is not None:
        pf.alignment = align


def shade(element, color="F2F2F2"):
    shd = OxmlElement('w:shd')
    shd.set(qn('w:val'), 'clear')
    shd.set(qn('w:color'), 'auto')
    shd.set(qn('w:fill'), color)
    element.append(shd)


# ---------------- 结构化转换 ----------------
class Md2Docx:
    def __init__(self):
        self.doc = Document()
        self._setup_page()
        self._setup_styles()
        self.has_h1 = False

    def _setup_page(self):
        sec = self.doc.sections[0]
        sec.page_width, sec.page_height = Cm(21), Cm(29.7)
        sec.top_margin = Cm(2.4)
        sec.bottom_margin = Cm(2.2)
        sec.left_margin = Cm(2.6)
        sec.right_margin = Cm(2.4)

    def _setup_styles(self):
        st = self.doc.styles['Normal']
        st.font.name = BODY_ASCII
        st.font.size = Pt(10.5)
        st.element.rPr.rFonts.set(qn('w:eastAsia'), BODY_EAST)

    # ---------- 标题 ----------
    def add_h1(self, text):
        self.has_h1 = True
        self._add_heading(text, 22, (0x1F, 0x38, 0x64),
                          align=WD_ALIGN_PARAGRAPH.CENTER, before=0, after=18)

    def add_h2(self, text):
        self._add_heading(text, 16, (0x1F, 0x38, 0x64),
                          align=WD_ALIGN_PARAGRAPH.LEFT,
                          before=24, after=12, page_break=True)

    def add_h3(self, text):
        self._add_heading(text, 13, (0x2E, 0x74, 0xB5), before=14, after=6)

    def add_h4(self, text):
        self._add_heading(text, 11.5, (0x2E, 0x74, 0xB5), before=10, after=4)

    def _add_heading(self, text, size, color, align=None, before=12, after=6,
                     page_break=False):
        p = self.doc.add_paragraph()
        style_para(p, before=before, after=after, line=1.3, align=align)
        if page_break:
            p.paragraph_format.page_break_before = True
        run = p.add_run(text)
        set_font(run, BODY_EAST, BODY_ASCII, size, bold=True, color=color)
        ppr = p._element.get_or_add_pPr()
        ol = OxmlElement('w:outlineLvl')
        ol.set(qn('w:val'), str(HEADING_SIZES.get(size, 3)))
        ppr.append(ol)
        return p

    def add_h2n(self, text):
        """标题内含反引号 → 用带格式的 runs 渲染"""
        p = self.doc.add_paragraph()
        style_para(p, before=24, after=12, line=1.3,
                   align=WD_ALIGN_PARAGRAPH.LEFT)
        p.paragraph_format.page_break_before = True
        add_inline(p, text, base_size=16, bold=True,
                   color=(0x1F, 0x38, 0x64))
        for r in p.runs:
            r.font.bold = True
            if not r.font.color or r.font.color.rgb is None:
                r.font.color.rgb = RGBColor(0x1F, 0x38, 0x64)
        ppr = p._element.get_or_add_pPr()
        ol = OxmlElement('w:outlineLvl')
        ol.set(qn('w:val'), '1')
        ppr.append(ol)
        return p

    # ---------- 正文 ----------
    def add_body(self, text):
        p = self.doc.add_paragraph()
        style_para(p, after=6, line=1.4)
        add_inline(p, text)

    def add_note(self, text):
        p = self.doc.add_paragraph()
        style_para(p, before=4, after=8, line=1.35)
        pf = p.paragraph_format
        pf.left_indent = Pt(10)
        pf.right_indent = Pt(6)
        add_inline(p, text, color=(0x40, 0x40, 0x40))
        shade(p._element.get_or_add_pPr(), "FFF2CC")

    def add_bullet(self, text, level=0):
        p = self.doc.add_paragraph()
        style_para(p, after=3, line=1.35)
        p.paragraph_format.left_indent = Pt(18 + level * 18)
        p.paragraph_format.first_line_indent = Pt(-12)
        run = p.add_run("• ")
        set_font(run, BODY_EAST, BODY_ASCII, 10.5, bold=True,
                 color=(0x1F, 0x38, 0x64))
        add_inline(p, text)

    def add_numbered(self, num, text):
        p = self.doc.add_paragraph()
        style_para(p, after=3, line=1.35)
        p.paragraph_format.left_indent = Pt(18)
        p.paragraph_format.first_line_indent = Pt(-12)
        run = p.add_run("%d. " % num)
        set_font(run, BODY_EAST, BODY_ASCII, 10.5, bold=True,
                 color=(0x1F, 0x38, 0x64))
        add_inline(p, text)

    def add_code(self, code):
        p = self.doc.add_paragraph()
        style_para(p, before=2, after=8, line=1.15)
        pf = p.paragraph_format
        pf.left_indent = Pt(8)
        pf.right_indent = Pt(4)
        run = p.add_run(code)
        set_font(run, CODE_EAST, CODE_ASCII, 8.5)
        shade(p._element.get_or_add_pPr(), "F2F2F2")

    # ---------- 表格 ----------
    def add_table(self, rows):
        ncols = max(len(r) for r in rows)
        table = self.doc.add_table(rows=len(rows), cols=ncols)
        table.style = 'Table Grid'
        table.alignment = WD_TABLE_ALIGNMENT.CENTER
        widths = self._auto_widths(rows, ncols)
        for ri, row in enumerate(rows):
            for ci in range(ncols):
                cell = table.cell(ri, ci)
                text = row[ci] if ci < len(row) else ""
                cell.width = Cm(widths[ci])
                p = cell.paragraphs[0]
                style_para(p, before=2, after=2, line=1.2)
                add_inline(p, text, base_size=9.5, bold=(ri == 0))
                if ri == 0:
                    shade(cell._element.get_or_add_tcPr(), "D9E2F3")
                    for r in p.runs:
                        r.font.bold = True
                elif ri % 2 == 0:
                    shade(cell._element.get_or_add_tcPr(), "F7F9FC")
        return table

    def _auto_widths(self, rows, ncols):
        total = 15.8
        maxlens = []
        for ci in range(ncols):
            m = 1
            for r in rows:
                if ci < len(r):
                    plain = re.sub(r'[`*]', '', r[ci])
                    m = max(m, min(len(plain) + 2, 46))
            maxlens.append(m)
        s = sum(maxlens)
        return [total * m / s for m in maxlens]


# ---------------- markdown 解析 ----------------
def parse_and_render(md_path, converter):
    with open(md_path, encoding="utf-8") as f:
        lines = f.read().split("\n")

    i = 0
    in_code = False
    code_buf = []
    para_buf = []
    tables_buf = []
    list_buf = []

    def flush_para():
        nonlocal para_buf
        if para_buf:
            text = " ".join(para_buf).strip()
            if text:
                converter.add_body(text)
            para_buf = []

    def flush_list():
        nonlocal list_buf
        for item in list_buf:
            lvl, t = item[0], item[1]
            num = item[2] if len(item) > 2 else 0
            if num:
                converter.add_numbered(num, t)
            else:
                converter.add_bullet(t, lvl)
        list_buf = []

    def flush_table():
        nonlocal tables_buf
        if tables_buf:
            cleaned = []
            for r in tables_buf:
                cells = [c.strip() for c in r]
                if any(cells):
                    # 去掉尾部空单元格
                    while cells and not cells[-1]:
                        cells.pop()
                    if cells:
                        cleaned.append(cells)
            if len(cleaned) >= 2:
                converter.add_table(cleaned)
            elif cleaned:
                converter.add_body(" ".join(cleaned[0]))
            tables_buf = []

    def flush_all():
        flush_para(); flush_list(); flush_table()

    while i < len(lines):
        line = lines[i]
        s = line.strip()

        if s.startswith("```"):
            if in_code:
                converter.add_code("\n".join(code_buf))
                code_buf = []
                in_code = False
            else:
                flush_all()
                in_code = True
            i += 1
            continue
        if in_code:
            code_buf.append(line)
            i += 1
            continue

        if s == "":
            flush_all()
            i += 1
            continue

        if s.startswith("|"):
            flush_para(); flush_list()
            if not re.match(r'^[\s|:-]+$', s):
                tables_buf.append([c.strip() for c in s.strip("|").split("|")])
            i += 1
            continue
        elif tables_buf:
            flush_table()

        m = re.match(r'^(#{1,4})\s+(.*)$', s)
        if m:
            flush_all()
            level = len(m.group(1))
            text = m.group(2).strip()
            if level == 1:
                converter.add_h1(text)
            elif level == 2:
                if '`' in text or '**' in text:
                    converter.add_h2n(text)
                else:
                    converter.add_h2(text)
            elif level == 3:
                converter.add_h3(text)
            else:
                converter.add_h4(text)
            i += 1
            continue

        if s.startswith("---") or s.startswith("***"):
            flush_all()
            i += 1
            continue

        if re.match(r'^-\s+\[.*\]\(#.*\)$', s):
            flush_para()
            text = re.sub(r'\[([^\]]*)\]\([^)]*\)', r'\1', s[2:])
            list_buf.append((0, text, 0))
            i += 1
            continue

        m = re.match(r'^(\s*)([-*]|\d+\.)\s+(.*)$', line)
        if m:
            flush_para()
            lvl = 1 if len(m.group(1)) >= 2 else 0
            marker = m.group(2)
            body = m.group(3).strip()
            if marker[0].isdigit():
                num = int(marker.rstrip('.'))
                list_buf.append((lvl, body, num))
            else:
                list_buf.append((lvl, body, 0))
            i += 1
            continue

        if s.startswith(">"):
            flush_all()
            converter.add_note(s.lstrip("> ").strip())
            i += 1
            continue

        para_buf.append(s)
        i += 1

    flush_all()


# ---------------- 主流程 ----------------
def main():
    conv = Md2Docx()
    parse_and_render(SRC, conv)
    conv.doc.save(DST)
    print("saved:", DST)

    d = Document(DST)
    print("paragraphs:", len(d.paragraphs), "tables:", len(d.tables))
    print("h1:", [p.text for p in d.paragraphs
                  if p.runs and p.runs[0].font.size
                  and p.runs[0].font.size.pt == 22])
    print("table separator rows remaining:",
          sum(1 for t in d.tables
              for r in t.rows if r.cells[0].text.strip() == "---"))
    print("bold runs:", sum(1 for p in d.paragraphs
                            for r in p.runs if r.font.bold))


if __name__ == "__main__":
    main()
