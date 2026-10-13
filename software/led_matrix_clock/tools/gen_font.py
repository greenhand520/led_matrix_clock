#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""七段数码管样式字模生成器（大 4x7 / 小 3x5）。

生成 src/display/font_data.cpp；行式格式：每行 1 字节，bit0 = 最左列。
段位定义：a(顶横) f(左上竖) b(右上竖) g(中横) e(左下竖) c(右下竖) d(底横)。

用法：python3 tools/gen_font.py [输出文件]   （缺省输出到 stdout）
"""
import sys

# (a, f, b, g, e, c, d) 七段开关
SEG7 = {
    '0': (1, 1, 1, 0, 1, 1, 1),
    '1': (0, 0, 1, 0, 0, 1, 0),
    '2': (1, 0, 1, 1, 1, 0, 1),
    '3': (1, 0, 1, 1, 0, 1, 1),
    '4': (0, 1, 1, 1, 0, 1, 0),
    '5': (1, 1, 0, 1, 0, 1, 1),
    '6': (1, 1, 0, 1, 1, 1, 1),
    '7': (1, 0, 1, 0, 0, 1, 0),
    '8': (1, 1, 1, 1, 1, 1, 1),
    '9': (1, 1, 1, 1, 0, 1, 1),
    'A': (1, 1, 1, 1, 1, 1, 0),
    'P': (1, 1, 1, 1, 1, 0, 0),
}

# 非七段小符号（3x5），行自上而下、列自左而右
SMALL_ART = {
    '%': ("#.#", "..#", ".#.", "#..", "#.#"),
    '-': ("...", "...", "###", "...", "..."),
    '\x80': ("#.#", "...", "###", "#..", "###"),  # 0x80 = 摄氏度 °C 组合符号
}

SMALL_ORDER = "0123456789%-A P\x80".replace(" ", "")  # 'A'、'P' 单列


def art_rows(art):
    rows = []
    for line in art:
        v = 0
        for c, ch in enumerate(line):
            if ch == '#':
                v |= 1 << c
        rows.append(v)
    return rows


def seg_rows(segs, w):
    """七段 → 行字节；w=4 竖段占 2 行，w=3 竖段占 1 行。"""
    a, f, b, g, e, c, d = segs
    H = (1 << w) - 1

    def stem(l, r):
        return (1 if l else 0) | ((1 << (w - 1)) if r else 0)

    n = 2 if w == 4 else 1
    rows = [H if a else 0]
    rows += [stem(f, b)] * n
    rows.append(H if g else 0)
    rows += [stem(e, c)] * n
    rows.append(H if d else 0)
    return rows


def emit():
    out = []
    out.append("// 自动生成文件 —— 由 tools/gen_font.py 生成，请勿手改")
    out.append("// 字模格式：行式，每行 1 字节，bit0 = 最左列（见布局稿 §1）")
    out.append('#include "display/font_data.h"')
    out.append("")
    out.append("namespace {")
    out.append("")
    out.append("// 大数字 4x7：'0'..'9'，每字符 7 行（七段样式）")
    out.append("const uint8_t kBig4x7[10][7] = {")
    for d in "0123456789":
        rows = seg_rows(SEG7[d], 4)
        body = ", ".join("0x%02X" % v for v in rows)
        out.append("    {%s},  // '%s'" % (body, d))
    out.append("};")
    out.append("")
    out.append("// 小字 3x5：{码值, 5 行}（七段数字 + 自绘符号）")
    out.append("struct SmallGlyph {")
    out.append("  uint8_t code;")
    out.append("  uint8_t rows[5];")
    out.append("};")
    out.append("const SmallGlyph kSmall3x5[] = {")
    for ch in "0123456789":
        rows = seg_rows(SEG7[ch], 3)
        out.append("    {0x%02X, {%s}},  // '%s'" % (ord(ch), ", ".join("0x%02X" % v for v in rows), ch))
    for ch in ('%', '-', 'A', 'P', '\x80'):
        if ch in SMALL_ART:
            rows = art_rows(SMALL_ART[ch])
            label = "°C" if ch == '\x80' else ch
        else:
            rows = seg_rows(SEG7[ch], 3)
            label = ch
        out.append("    {0x%02X, {%s}},  // '%s'" % (ord(ch), ", ".join("0x%02X" % v for v in rows), label))
    out.append("};")
    out.append("")
    out.append("}  // namespace")
    out.append("")
    out.append("const uint8_t* bigDigit4x7(char ch) {")
    out.append("  if (ch < '0' || ch > '9') {")
    out.append("    return nullptr;")
    out.append("  }")
    out.append("  return kBig4x7[ch - '0'];")
    out.append("}")
    out.append("")
    out.append("const uint8_t* smallGlyph3x5(char ch) {")
    out.append("  for (const SmallGlyph& g : kSmall3x5) {")
    out.append("    if (g.code == static_cast<uint8_t>(ch)) {")
    out.append("      return g.rows;")
    out.append("    }")
    out.append("  }")
    out.append("  return nullptr;")
    out.append("}")
    out.append("")
    return "\n".join(out)


if __name__ == "__main__":
    text = emit()
    if len(sys.argv) > 1:
        with open(sys.argv[1], "w", encoding="utf-8") as f:
            f.write(text)
    else:
        sys.stdout.write(text)
