"""Generate seamless tiling textures for the Windmill Farm scene.

Pure stdlib: PNGs are written by hand with zlib + struct. Every pattern wraps
at the edges so GL_REPEAT shows no seam.
"""
import math
import os
import random
import struct
import sys
import zlib

SIZE = 256
OUT = sys.argv[1]


# --------------------------------------------------------------------- PNG
def write_png(path, w, h, pixels):
    """pixels: list of rows, each row a list of (r,g,b) floats in 0..1"""
    raw = bytearray()
    for row in pixels:
        raw.append(0)  # filter type 0
        for (r, g, b) in row:
            raw.append(max(0, min(255, int(r * 255 + 0.5))))
            raw.append(max(0, min(255, int(g * 255 + 0.5))))
            raw.append(max(0, min(255, int(b * 255 + 0.5))))

    def chunk(tag, data):
        return (struct.pack('>I', len(data)) + tag + data +
                struct.pack('>I', zlib.crc32(tag + data) & 0xffffffff))

    png = b'\x89PNG\r\n\x1a\n'
    png += chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
    png += chunk(b'IDAT', zlib.compress(bytes(raw), 9))
    png += chunk(b'IEND', b'')
    with open(path, 'wb') as f:
        f.write(png)
    return os.path.getsize(path)


# ------------------------------------------------------------------- noise
def lattice(cx, cy, seed):
    rnd = random.Random(seed)
    return [[rnd.random() for _ in range(cx)] for _ in range(cy)]


def noise_field(w, h, cx, cy, seed):
    """Value noise on a cx*cy lattice, bilinear + smoothstep, wrapping."""
    g = lattice(cx, cy, seed)
    out = [[0.0] * w for _ in range(h)]
    for y in range(h):
        fy = y / h * cy
        iy = int(fy)
        ty = fy - iy
        ty = ty * ty * (3 - 2 * ty)
        y0, y1 = iy % cy, (iy + 1) % cy
        for x in range(w):
            fx = x / w * cx
            ix = int(fx)
            tx = fx - ix
            tx = tx * tx * (3 - 2 * tx)
            x0, x1 = ix % cx, (ix + 1) % cx
            a = g[y0][x0] * (1 - tx) + g[y0][x1] * tx
            b = g[y1][x0] * (1 - tx) + g[y1][x1] * tx
            out[y][x] = a * (1 - ty) + b * ty
    return out


def fbm(w, h, base, seed, octaves=4, aspect=1.0):
    """Fractal sum of value noise. aspect > 1 stretches features vertically."""
    total = [[0.0] * w for _ in range(h)]
    amp, norm = 1.0, 0.0
    for o in range(octaves):
        cx = max(2, int(base * (2 ** o)))
        cy = max(2, int(base * (2 ** o) / aspect))
        f = noise_field(w, h, cx, cy, seed + o * 977)
        for y in range(h):
            trow, frow = total[y], f[y]
            for x in range(w):
                trow[x] += frow[x] * amp
        norm += amp
        amp *= 0.5
    for y in range(h):
        row = total[y]
        for x in range(w):
            row[x] /= norm
    return total


def mix(c0, c1, t):
    return (c0[0] + (c1[0] - c0[0]) * t,
            c0[1] + (c1[1] - c0[1]) * t,
            c0[2] + (c1[2] - c0[2]) * t)


def shade(c, k):
    return (c[0] * k, c[1] * k, c[2] * k)


# ---------------------------------------------------------------- patterns
def make_grass():
    n = fbm(SIZE, SIZE, 4, 11, 5)
    fine = fbm(SIZE, SIZE, 32, 77, 2)
    dark, light = (0.21, 0.38, 0.15), (0.46, 0.66, 0.29)
    px = []
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            t = n[y][x] * 0.75 + fine[y][x] * 0.25
            c = mix(dark, light, t)
            # sparse brighter blades
            if fine[y][x] > 0.80:
                c = shade(c, 1.18)
            row.append(c)
        px.append(row)
    return px


def make_leaves():
    n = fbm(SIZE, SIZE, 6, 23, 4)
    fine = fbm(SIZE, SIZE, 24, 91, 2)
    dark, light = (0.09, 0.24, 0.10), (0.30, 0.54, 0.24)
    px = []
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            t = n[y][x] * 0.6 + fine[y][x] * 0.4
            c = mix(dark, light, t)
            if fine[y][x] < 0.22:
                c = shade(c, 0.78)          # shadow pockets between clumps
            row.append(c)
        px.append(row)
    return px


def make_bark():
    # aspect > 1 stretches the noise vertically into fibres
    n = fbm(SIZE, SIZE, 8, 31, 4, aspect=8.0)
    fine = fbm(SIZE, SIZE, 24, 53, 2, aspect=10.0)
    dark, light = (0.21, 0.13, 0.07), (0.49, 0.34, 0.20)
    px = []
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            t = n[y][x] * 0.7 + fine[y][x] * 0.3
            c = mix(dark, light, t)
            # deep vertical cracks
            if n[y][x] < 0.30:
                c = shade(c, 0.62)
            row.append(c)
        px.append(row)
    return px


def make_wood():
    """Vertical planks with grain, for the windmill head, fence and wheel."""
    planks = 6
    pw = SIZE / planks
    grain = fbm(SIZE, SIZE, 6, 41, 4, aspect=12.0)
    rnd = random.Random(404)
    tone = [0.86 + rnd.random() * 0.28 for _ in range(planks)]
    dark, light = (0.36, 0.23, 0.12), (0.68, 0.49, 0.28)
    px = []
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            p = int(x / pw) % planks
            edge = (x % pw)
            c = mix(dark, light, grain[y][x])
            c = shade(c, tone[p])
            # gap between planks, on both sides so it wraps
            if edge < 2.0 or edge > pw - 2.0:
                c = shade(c, 0.45)
            row.append(c)
        px.append(row)
    return px


def make_brick():
    rows, cols = 8, 4
    bh, bw = SIZE / rows, SIZE / cols
    mortar_t = 3.0
    n = fbm(SIZE, SIZE, 16, 61, 3)
    rnd = random.Random(7)
    tint = {}
    mortar = (0.80, 0.78, 0.72)
    px = []
    for y in range(SIZE):
        row = []
        r = int(y / bh)
        offset = (bw * 0.5) if (r % 2) else 0.0
        for x in range(SIZE):
            ey = y % bh
            ex = (x + offset) % bw
            if ey < mortar_t or ex < mortar_t:
                c = mix(mortar, shade(mortar, 0.88), n[y][x])
            else:
                key = (r % rows, int((x + offset) / bw) % cols)
                if key not in tint:
                    tint[key] = 0.82 + rnd.random() * 0.34
                c = mix((0.50, 0.23, 0.17), (0.72, 0.39, 0.29), n[y][x])
                c = shade(c, tint[key])
            row.append(c)
        px.append(row)
    return px


def make_roof():
    """Overlapping curved roof tiles."""
    rows, cols = 8, 8
    th, tw = SIZE / rows, SIZE / cols
    n = fbm(SIZE, SIZE, 16, 83, 3)
    rnd = random.Random(19)
    tint = {}
    px = []
    for y in range(SIZE):
        row = []
        r = int(y / th)
        offset = (tw * 0.5) if (r % 2) else 0.0
        for x in range(SIZE):
            ey = (y % th) / th          # 0 at top of tile, 1 at bottom
            ex = ((x + offset) % tw) / tw
            key = (r % rows, int((x + offset) / tw) % cols)
            if key not in tint:
                tint[key] = 0.86 + rnd.random() * 0.26
            c = mix((0.44, 0.17, 0.12), (0.72, 0.34, 0.24), n[y][x])
            c = shade(c, tint[key])
            # barrel shading across the tile, darker at the edges
            curve = 0.72 + 0.42 * math.sin(math.pi * ex)
            c = shade(c, curve)
            # shadow under the overlapping course above
            if ey < 0.16:
                c = shade(c, 0.58)
            row.append(c)
        px.append(row)
    return px


def make_stone():
    """Rendered plaster over blockwork, for the windmill tower."""
    n = fbm(SIZE, SIZE, 5, 101, 5)
    fine = fbm(SIZE, SIZE, 40, 131, 2)
    rows, cols = 6, 3
    bh, bw = SIZE / rows, SIZE / cols
    base_d, base_l = (0.62, 0.59, 0.53), (0.88, 0.86, 0.80)
    px = []
    for y in range(SIZE):
        row = []
        r = int(y / bh)
        offset = (bw * 0.5) if (r % 2) else 0.0
        for x in range(SIZE):
            t = n[y][x] * 0.8 + fine[y][x] * 0.2
            c = mix(base_d, base_l, t)
            ey = y % bh
            ex = (x + offset) % bw
            if ey < 2.0 or ex < 2.0:        # faint block seams
                c = shade(c, 0.86)
            row.append(c)
        px.append(row)
    return px


def make_metal():
    """Brushed grey, for lamp posts and hubs."""
    streak = fbm(SIZE, SIZE, 6, 211, 4, aspect=0.08)   # stretched horizontally
    blotch = fbm(SIZE, SIZE, 8, 233, 3)
    dark, light = (0.19, 0.19, 0.23), (0.46, 0.47, 0.52)
    px = []
    for y in range(SIZE):
        row = []
        for x in range(SIZE):
            t = streak[y][x] * 0.75 + blotch[y][x] * 0.25
            row.append(mix(dark, light, t))
        px.append(row)
    return px


TEXTURES = [
    ('grass.png',  make_grass),
    ('leaves.png', make_leaves),
    ('bark.png',   make_bark),
    ('wood.png',   make_wood),
    ('brick.png',  make_brick),
    ('roof.png',   make_roof),
    ('stone.png',  make_stone),
    ('metal.png',  make_metal),
]

if __name__ == '__main__':
    os.makedirs(OUT, exist_ok=True)
    for name, fn in TEXTURES:
        path = os.path.join(OUT, name)
        size = write_png(path, SIZE, SIZE, fn())
        print('{:12s} {:>7,} bytes'.format(name, size))
    print('done ->', OUT)
