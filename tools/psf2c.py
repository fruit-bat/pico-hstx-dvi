#!/usr/bin/env python3
"""Turns Linux console fonts (PSF 1 or 2, optionally gzipped) into C, for
hstx_dvi_font.h: the printable ASCII characters (32 to 126) of each, then the
128 extra glyphs in EXTRAS (line drawing, blocks, Latin-1 letters...), found
by the font's Unicode table, as rows of bytes with the leftmost pixel in the
top bit. The few simple shapes a font lacks are drawn.

  tools/psf2c.py out.c NAME=file.psf.gz[:note] ...

Each NAME becomes `const hstx_dvi_font_t hstx_dvi_font_NAME`, built only if
HSTX_DVI_FONT_NAME (in capitals) is 1, as hstx_dvi_font.h defaults it to, so
it can be left out to save flash. The note, if given, goes in a comment above
it, e.g. where the font came from and its licence.
"""
import gzip
import shlex
import struct
import sys

FIRST, LAST = 32, 126


def read_psf(path):
    data = open(path, 'rb').read()
    if path.endswith('.gz'):
        data = gzip.decompress(data)
    if data[:2] == b'\x36\x04':
        mode, size = data[2], data[3]
        count = 512 if mode & 1 else 256
        w, h, bpg, off = 8, size, size, 4
        glyphs = [data[off + i * bpg: off + (i + 1) * bpg] for i in range(count)]
        table = {}
        if mode & 2:
            p = off + count * bpg
            for i in range(count):
                while True:
                    (u,) = struct.unpack_from('<H', data, p)
                    p += 2
                    if u == 0xffff:
                        break
                    if u != 0xfffe:
                        table.setdefault(u, i)
    elif data[:4] == b'\x72\xb5\x4a\x86':
        _, hsize, flags, count, bpg, h, w = struct.unpack_from('<7I', data, 4)
        glyphs = [data[hsize + i * bpg: hsize + (i + 1) * bpg] for i in range(count)]
        table = {}
        if flags & 1:
            p = hsize + count * bpg
            for i in range(count):
                while data[p] != 0xff:
                    if data[p] == 0xfe:
                        p += 1
                        continue
                    # One UTF-8 character
                    b = data[p]
                    n = 1 if b < 0x80 else 2 if b < 0xe0 else 3 if b < 0xf0 else 4
                    table.setdefault(ord(data[p:p + n].decode('utf-8')), i)
                    p += n
                p += 1
    else:
        raise SystemExit(f'{path}: not a PSF font')
    if not table:
        table = {c: c for c in range(min(len(glyphs), 256))}
    return w, h, glyphs, table


# The glyphs after ASCII, for character codes 128 to 255, in this order: the
# DEC special graphics, double and rounded box lines, blocks, arrows, quotes
# and dashes, and Latin-1's accented letters. hstx_dvi_vt.c's character map
# puts them in the cells.
EXTRAS = ('◆▒°±┘┐┌└┼⎺⎻─⎼⎽├┤┴┬│≤≥π≠£·'
          '═║╔╗╚╝╠╣╦╩╬'
          '╭╮╯╰'
          '█▀▄▌▐░▓■'
          '←↑→↓▲▼'
          '•…‘’“”–—€™'
          + ''.join(chr(c) for c in range(0xc0, 0x100)))
assert len(EXTRAS) == 128


def to_grid(g, w, h):
    bpr = (w + 7) // 8
    return [[(g[y * bpr + x // 8] >> (7 - x % 8)) & 1 for x in range(w)] for y in range(h)]


def from_grid(grid, w, h):
    bpr = (w + 7) // 8
    out = bytearray(h * bpr)
    for y in range(h):
        for x in range(w):
            if grid[y][x]:
                out[y * bpr + x // 8] |= 0x80 >> (x % 8)
    return bytes(out)


def synth(ch, w, h, glyph):
    """Draws a glyph the font lacks, from simple shapes or its other glyphs,
    or returns None"""
    cp = ord(ch)
    grid = [[0] * w for _ in range(h)]
    if cp in (0x2580, 0x2584, 0x258c, 0x2590):  # half blocks
        for y in range(h):
            for x in range(w):
                grid[y][x] = {0x2580: y < h // 2, 0x2584: y >= h // 2,
                              0x258c: x < w // 2, 0x2590: x >= w // 2}[cp]
        return from_grid(grid, w, h)
    if cp == 0x2593:  # dark shade, the light one turned inside out
        light = glyph(0x2591)
        return light and from_grid([[1 - p for p in row] for row in to_grid(light, w, h)], w, h)
    if cp == 0x25c6:  # a diamond
        r = min(w, h) / 2 - 0.5
        cx, cy = (w - 1) / 2, (h - 1) / 2
        for y in range(h):
            for x in range(w):
                grid[y][x] = abs(x - cx) + abs(y - cy) <= r
        return from_grid(grid, w, h)
    if cp in (0x23ba, 0x23bb, 0x23bc, 0x23bd):  # scan lines, as thick as ─
        line = glyph(0x2500)
        if not line:
            return None
        rows = [y for y, row in enumerate(to_grid(line, w, h)) if any(row)]
        t = len(rows) or 1
        at = {0x23ba: 0, 0x23bb: h // 4, 0x23bc: 3 * h // 4, 0x23bd: h - t}[cp]
        at = max(0, min(h - t, at - (t // 2 if 0 < at < h - t else 0)))
        for y in range(at, at + t):
            grid[y] = [1] * w
        return from_grid(grid, w, h)
    return None


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    out = ['// Made by tools/psf2c.py from Linux console fonts: see hstx_dvi_font.h.',
           '// Printable ASCII, then the extra glyphs of hstx_dvi_font_extras, a glyph',
           '// after another, each h rows of (w + 7) / 8 bytes, the leftmost pixel in the',
           '// top bit. Made with this, as one command:', '//',
           '//   ' + '\n//     '.join(shlex.quote(a) for a in ['tools/psf2c.py'] + sys.argv[1:]), '',
           '#include "hstx_dvi_font.h"', '',
           '// The extra glyphs\' code points, for character codes 128 to 255',
           'const uint16_t hstx_dvi_font_extras[HSTX_DVI_FONT_EXTRAS] = {']
    for i in range(0, len(EXTRAS), 8):
        part = EXTRAS[i:i + 8]
        out.append('\t' + ' '.join(f'0x{ord(c):04x},' for c in part) + f' // {part}')
    out += ['};', '']
    for arg in sys.argv[2:]:
        name, rest = arg.split('=', 1)
        path, _, note = rest.partition(':')
        w, h, glyphs, table = read_psf(path)
        bpr = (w + 7) // 8

        def glyph(cp):
            return glyphs[table[cp]][:h * bpr] if cp in table else None

        out.append(f'#if HSTX_DVI_FONT_{name.upper()}')
        if note:
            out.append(f'// {note}')
        out.append(f'// {w}x{h}, from {path.rsplit("/", 1)[-1]}')
        out.append(f'static const uint8_t _{name}_bits[] = {{')
        drawn = []
        for ch in [chr(c) for c in range(FIRST, LAST + 1)] + list(EXTRAS):
            g = glyph(ord(ch))
            if g is None and ord(ch) > LAST:
                g = synth(ch, w, h, glyph)
                drawn.append(ch)
            if g is None:
                raise SystemExit(f'{path}: no glyph for {ch!r}')
            rows = ', '.join(f'0x{b:02x}' for b in g)
            out.append(f'\t{rows}, // {ch if ch != chr(92) else "backslash"}')
        out.append('};')
        if drawn:
            out.append(f'// Drawn by psf2c.py, as the font has none: {"".join(drawn)}')
        count = LAST - FIRST + 1 + len(EXTRAS)
        out.append(f'const hstx_dvi_font_t hstx_dvi_font_{name} = {{ "{name}", {w}, {h}, {FIRST}, {count}, _{name}_bits }};')
        out.append(f'#endif // HSTX_DVI_FONT_{name.upper()}')
        out.append('')
    open(sys.argv[1], 'w').write('\n'.join(out))


if __name__ == '__main__':
    main()
