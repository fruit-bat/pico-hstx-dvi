#!/usr/bin/env python3
"""Turns Linux console fonts (PSF 1 or 2, optionally gzipped) into C, for
hstx_dvi_font.h: the printable ASCII characters (32 to 126) of each, found by
the font's Unicode table, as rows of bytes with the leftmost pixel in the top
bit.

  tools/psf2c.py out.c NAME=file.psf.gz[:note] ...

Each NAME becomes `const hstx_dvi_font_t hstx_dvi_font_NAME`. The note, if
given, goes in a comment above it, e.g. where the font came from and its
licence.
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


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    out = ['// Made by tools/psf2c.py from Linux console fonts: see hstx_dvi_font.h.',
           '// Printable ASCII only, a glyph after another, each h rows of (w + 7) / 8',
           '// bytes, the leftmost pixel in the top bit. Made with this, as one command:', '//',
           '//   ' + '\n//     '.join(shlex.quote(a) for a in ['tools/psf2c.py'] + sys.argv[1:]), '',
           '#include "hstx_dvi_font.h"', '']
    for arg in sys.argv[2:]:
        name, rest = arg.split('=', 1)
        path, _, note = rest.partition(':')
        w, h, glyphs, table = read_psf(path)
        bpr = (w + 7) // 8
        if note:
            out.append(f'// {note}')
        out.append(f'// {w}x{h}, from {path.rsplit("/", 1)[-1]}')
        out.append(f'static const uint8_t _{name}_bits[] = {{')
        for c in range(FIRST, LAST + 1):
            if c not in table:
                raise SystemExit(f'{path}: no glyph for {chr(c)!r}')
            g = glyphs[table[c]]
            rows = ', '.join(f'0x{b:02x}' for b in g[:h * bpr])
            out.append(f'\t{rows}, // {chr(c) if c != 92 else "backslash"}')
        out.append('};')
        out.append(f'const hstx_dvi_font_t hstx_dvi_font_{name} = {{ "{name}", {w}, {h}, {FIRST}, {LAST - FIRST + 1}, _{name}_bits }};')
        out.append('')
    open(sys.argv[1], 'w').write('\n'.join(out))


if __name__ == '__main__':
    main()
