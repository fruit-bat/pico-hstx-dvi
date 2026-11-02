#!/usr/bin/env python3
# Generates the models in ../src/ship_model.c from their corners.
# The faces and edges of each hull are found from the convex hull of its corners,
# so each edge knows the two faces it joins, for hiding the edges at the back.
# Details such as engines are added as shapes on the faces they sit on, or as
# lines, which are shown when the faces they are fixed to are.
#
# python3 art/ship_model.py

import itertools
import math
import os

# Colour 0 is the model's own colour, then as PART_ in main.c
ENGINE = 1
VENT = 2
TRIM = 3


def sym(*pts):
	# Corners mirrored left to right, for the many models that are the same both sides
	out = []
	for x, y, z in pts:
		out.append((x, y, z))
		if x:
			out.append((-x, y, z))
	return out


def quad(x0, y0, x1, y1, z):
	# A rectangle across the back, for engines
	return [(x0, y0, z), (x1, y0, z), (x1, y1, z), (x0, y1, z)]


def engines(x0, y0, x1, y1, z):
	# A pair of engines, mirrored
	return [
		("right engine", quad(x0, y0, x1, y1, z), True, ENGINE),
		("left engine", quad(-x1, y0, -x0, y1, z), True, ENGINE),
	]


def ring(n, r, y, turn=0.5):
	# n corners round a circle across y
	return [(round(r * math.cos(2 * math.pi * (i + turn) / n)), y, round(r * math.sin(2 * math.pi * (i + turn) / n))) for i in range(n)]


# Each model has its name, colour and the corners of its hull, x right, y up,
# z forwards, and its details: (name, corners, a shape or just lines?, colour).
# The shapes follow the ships of the original game, though not exactly.
MODELS = {
	"cobra": {
		"name": "COBRA MK III", "rgb": (255, 255, 255),
		"hull": [
			(32, 0, 76), (-32, 0, 76), (0, 26, 24), (-120, -3, -8), (120, -3, -8),
			(-88, 16, -40), (88, 16, -40), (128, -8, -40), (-128, -8, -40),
			(0, 26, -40), (-32, -24, -40), (32, -24, -40),
		],
		"details": [
			("left engine", [(-36, 8, -40), (-8, 12, -40), (-8, -16, -40), (-36, -12, -40)], True, ENGINE),
			("right engine", [(8, 12, -40), (36, 8, -40), (36, -12, -40), (8, -16, -40)], True, ENGINE),
			("left vent", [(-80, 6, -40), (-88, 0, -40), (-80, -6, -40)], True, VENT),
			("right vent", [(80, 6, -40), (88, 0, -40), (80, -6, -40)], True, VENT),
			("gun", [(0, 0, 76), (0, 0, 90)], False, TRIM),
		],
	},
	# A cuboctahedron, with the docking slot on the square at the front
	"coriolis": {
		"name": "CORIOLIS STATION", "rgb": (255, 255, 255),
		"hull": [
			(s * 400, t * 400, 0) for s in (1, -1) for t in (1, -1)
		] + [
			(s * 400, 0, t * 400) for s in (1, -1) for t in (1, -1)
		] + [
			(0, s * 400, t * 400) for s in (1, -1) for t in (1, -1)
		],
		"details": [
			("slot", [(-100, 30, 400), (100, 30, 400), (100, -30, 400), (-100, -30, 400)], True, TRIM),
		],
	},
	# A pointed body with four fins at the back
	"missile": {
		"name": "MISSILE", "rgb": (255, 73, 73),
		"hull": [
			(0, 0, 68),
			(8, 8, 36), (-8, 8, 36), (-8, -8, 36), (8, -8, 36),
			(8, 8, -44), (-8, 8, -44), (-8, -8, -44), (8, -8, -44),
		],
		"details": [
			(f"fin {sx} {sy}", [(sx * 8, sy * 8, -20), (sx * 20, sy * 20, -44), (sx * 8, sy * 8, -44)], False, ENGINE)
			for sx, sy in ((1, 1), (-1, 1), (-1, -1), (1, -1))
		],
	},
	"sidewinder": {
		"name": "SIDEWINDER", "rgb": (255, 255, 0),
		"hull": sym((32, 0, 36), (64, 0, -28), (0, 16, -28), (0, -16, -28)),
		"details": [("engine", quad(-12, 6, 12, -6, -28), True, ENGINE)],
	},
	"viper": {
		"name": "VIPER", "rgb": (0, 219, 255),
		"hull": sym((0, 0, 72), (0, 16, 24), (0, -16, 24), (48, 0, -24), (24, 16, -24), (24, -16, -24)),
		"details": engines(4, 8, 16, -8, -24),
	},
	"mamba": {
		"name": "MAMBA", "rgb": (0, 255, 0),
		"hull": sym((0, 0, 64), (64, -8, -32), (32, 8, -32), (0, -8, -32)),
		"details": engines(8, 4, 24, -4, -32),
	},
	"krait": {
		"name": "KRAIT", "rgb": (255, 146, 0),
		"hull": sym((0, 0, 96), (0, 18, -48), (0, -18, -48), (90, 0, -3)),
		"details": [
			("right prong", [(60, 0, 30), (86, 0, 62)], False, TRIM),
			("left prong", [(-60, 0, 30), (-86, 0, 62)], False, TRIM),
		],
	},
	"adder": {
		"name": "ADDER", "rgb": (182, 182, 182),
		"hull": sym((18, 7, 40), (18, -7, 40), (36, 0, 0), (30, 7, -40), (30, -7, -40)),
		"details": engines(4, 4, 16, -4, -40),
	},
	"gecko": {
		"name": "GECKO", "rgb": (146, 255, 0),
		"hull": sym((10, -4, 47), (16, 8, -23), (66, 0, -3), (20, -14, -23)),
		"details": engines(4, 4, 12, -8, -23),
	},
	"cobra1": {
		"name": "COBRA MK I", "rgb": (255, 182, 0),
		"hull": sym((18, -1, 50), (66, 0, 7), (32, 12, -38), (54, -12, -38)),
		"details": engines(10, 4, 24, -6, -38) + [("gun", [(0, -1, 50), (0, -1, 64)], False, TRIM)],
	},
	"asp": {
		"name": "ASP MK II", "rgb": (219, 109, 255),
		"hull": sym((26, 7, 73), (26, -7, 73), (43, 14, 28), (43, -14, 28), (69, 0, 0), (43, 0, -45), (0, 9, -45), (0, -9, -45)),
		"details": [("engine", [(-14, 4, -45), (14, 4, -45), (14, -4, -45), (-14, -4, -45)], True, ENGINE)],
	},
	"python": {
		"name": "PYTHON", "rgb": (255, 73, 73),
		"hull": sym((0, 0, 224), (96, 0, -16), (0, 48, 48), (0, -48, 48), (0, 24, -112), (0, -24, -112), (48, 0, -112)),
		"details": engines(8, 8, 28, -8, -112),
	},
	"anaconda": {
		"name": "ANACONDA", "rgb": (0, 182, 109),
		"hull": sym((0, 70, -94), (70, 20, -94), (45, -60, -94), (0, 30, 200), (30, 10, 200), (20, -25, 200), (0, 0, 254)),
		"details": engines(10, 20, 30, -30, -94),
	},
	"boa": {
		"name": "BOA", "rgb": (109, 146, 255),
		"hull": sym((0, 0, 93), (0, 40, -87), (38, -25, -99), (0, 26, -17), (0, -30, -17), (40, 4, -40)),
		"details": [],
	},
	"ferdelance": {
		"name": "FER-DE-LANCE", "rgb": (255, 109, 182),
		"hull": sym((0, -14, 108), (40, -14, -4), (12, -14, -52), (40, 14, -4), (12, 2, -52), (0, 18, -20)),
		"details": [],
	},
	"moray": {
		"name": "MORAY", "rgb": (0, 146, 219),
		"hull": sym((15, 0, 65), (0, 18, -40), (60, 0, 0), (30, -27, -10), (0, -15, -40)),
		"details": [],
	},
	"constrictor": {
		"name": "CONSTRICTOR", "rgb": (255, 0, 255),
		"hull": sym((20, -7, 80), (54, -7, 40), (54, -7, -40), (20, 13, -40), (20, 13, 5)),
		"details": engines(4, 7, 14, -3, -40),
	},
	"cougar": {
		"name": "COUGAR", "rgb": (146, 146, 146),
		"hull": sym((0, 5, 67), (20, 0, 40), (40, 0, -40), (0, 14, -40), (0, -14, -40)),
		"details": [],
	},
	"shuttle": {
		"name": "ORBIT SHUTTLE", "rgb": (255, 219, 109),
		"hull": sym((0, 0, 35), (0, -17, 23), (17, 0, 23), (0, 18, 23), (20, 20, -27), (20, -20, -27)),
		"details": [("engine", quad(-8, 8, 8, -8, -27), True, ENGINE)],
	},
	"transporter": {
		"name": "TRANSPORTER", "rgb": (182, 219, 109),
		"hull": sym((30, 10, -26), (30, -6, -26), (25, 8, 24), (30, -6, 30)),
		"details": engines(6, 4, 20, -4, -26),
	},
	"worm": {
		"name": "WORM", "rgb": (146, 109, 255),
		"hull": sym((10, -10, 35), (20, 10, -15), (35, -10, -15)),
		"details": [],
	},
	"thargoid": {
		"name": "THARGOID", "rgb": (182, 0, 255),
		"hull": ring(8, 100, -8, 0.5) + ring(8, 50, 24, 0.5),
		"details": [],
	},
	"thargon": {
		"name": "THARGON", "rgb": (255, 109, 255),
		"hull": [(-9, 0, 40), (-9, -38, 12), (-9, -24, -32), (-9, 24, -32), (-9, 38, 12), (9, 0, -8), (9, -10, -15), (9, 10, -15)],
		"details": [],
	},
	"escapepod": {
		"name": "ESCAPE POD", "rgb": (255, 146, 146),
		"hull": [(-7, 0, 36), (-7, -14, -12), (-7, 14, -12), (21, 0, 0)],
		"details": [],
	},
	"canister": {
		"name": "CARGO CANISTER", "rgb": (219, 219, 219),
		"hull": [(x, round(24 * math.cos(2 * math.pi * k / 5 + math.pi / 2)), round(24 * math.sin(2 * math.pi * k / 5 + math.pi / 2))) for x in (24, -24) for k in range(5)],
		"details": [],
	},
	"asteroid": {
		"name": "ASTEROID", "rgb": (146, 109, 73),
		"hull": [(0, 80, 0), (0, -72, 10), (76, 6, 0), (-80, 0, 8), (10, 0, 84), (-6, 4, -78),
			(46, 46, 40), (-40, 50, -44), (50, -44, -40), (-46, -40, 46), (44, -10, 50), (-50, 30, 30)],
		"details": [],
	},
}

# The ships that fly round the station, in this order
SHIPS = ["sidewinder", "viper", "mamba", "krait", "adder", "gecko", "cobra1", "cobra", "asp", "python",
	"anaconda", "boa", "ferdelance", "moray", "constrictor", "cougar", "shuttle", "transporter", "worm",
	"thargoid", "thargon", "escapepod", "canister", "asteroid"]

# How close two faces have to be to flat to have no edge between them
CREASE = 0.999
# The most corners on a face or a detail, as WIRE_MAX_LOOP in ship_model.h
MAX_LOOP = 8
EPS = 1e-6


def sub(a, b):
	return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def cross(a, b):
	return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def dot(a, b):
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def norm(a):
	l = math.sqrt(dot(a, a))
	return (a[0] / l, a[1] / l, a[2] / l)


def hull_faces(pts):
	# Every plane through 3 corners with all the others behind it is a face
	faces = []
	for i, j, k in itertools.combinations(range(len(pts)), 3):
		n = cross(sub(pts[j], pts[i]), sub(pts[k], pts[i]))
		if dot(n, n) < EPS:
			continue
		n = norm(n)
		d = dot(n, pts[i])
		side = [dot(n, p) - d for p in pts]
		if all(s <= EPS for s in side):
			pass
		elif all(s >= -EPS for s in side):
			n, d = (-n[0], -n[1], -n[2]), -d
		else:
			continue
		on = frozenset(m for m, p in enumerate(pts) if abs(dot(n, p) - d) < EPS)
		if all(f[2] != on for f in faces):
			faces.append((n, d, on))
	return faces


def face_loop(pts, n, on):
	# The corners on a face in order round its middle
	on = list(on)
	c = [sum(pts[m][a] for m in on) / len(on) for a in range(3)]
	u = norm(sub(pts[on[0]], c))
	v = cross(n, u)
	on.sort(key=lambda m: math.atan2(dot(sub(pts[m], c), v), dot(sub(pts[m], c), u)))
	return on


def face_edges(loop):
	return [tuple(sorted((loop[i], loop[(i + 1) % len(loop)]))) for i in range(len(loop))]


def face_of(faces, p):
	return [i for i, f in enumerate(faces) if abs(dot(f[0], p) - f[1]) < EPS]


def outside(faces, p):
	return any(dot(f[0], p) - f[1] > EPS for f in faces)


def make_model(hull, details):
	verts = list(hull)
	faces = hull_faces(hull)
	loops = [face_loop(hull, n, on) for n, d, on in faces]
	edges = {}
	for fi, loop in enumerate(loops):
		assert len(loop) <= MAX_LOOP
		for e in face_edges(loop):
			edges.setdefault(e, []).append(fi)
	out_edges = []
	for (a, b), fs in sorted(edges.items()):
		assert len(fs) == 2, (a, b, fs)
		# Faces that are all but flat to each other are one face with a slight bend, so no edge
		if dot(faces[fs[0]][0], faces[fs[1]][0]) > CREASE:
			continue
		out_edges.append((a, b, fs[0], fs[1], 0))

	polys = []
	for name, corners, shape, colour in details:
		idx = []
		for p in corners:
			if p not in verts:
				verts.append(p)
			idx.append(verts.index(p))
		# The faces the corners on the hull share, such as the two either side of the gun.
		# Corners sticking out, such as the tips of fins, are left out.
		on = [set(face_of(faces, p)) for p in corners if face_of(faces, p) and not outside(faces, p)]
		fs = sorted(set.intersection(*on))
		assert 1 <= len(fs) <= 2, (name, fs)
		f0, f1 = fs[0], fs[-1]
		pairs = list(zip(idx, idx[1:])) + ([(idx[-1], idx[0])] if shape else [])
		for a, b in pairs:
			out_edges.append((a, b, f0, f1, colour))
		if shape:
			assert f0 == f1 and len(idx) <= MAX_LOOP
			polys.append((f0, colour, idx))
	radius = max(math.sqrt(dot(p, p)) for p in verts)
	return verts, faces, loops, out_edges, polys, radius


def emit_model(out, name, m):
	hull, details = m["hull"], m["details"]
	verts, faces, loops, edges, polys, radius = make_model(hull, details)
	out += [f"// {name}", f"static const WireVertex_t _{name}_vertices[{len(verts)}] = {{"]
	out += [f"\t{{ {x}, {y}, {z} }}," for x, y, z in verts]
	out += ["};", "", f"static const WireFace_t _{name}_faces[{len(faces)}] = {{"]
	for (n, d, on), loop in zip(faces, loops):
		out.append(f"\t{{ {n[0]:.4f}f, {n[1]:.4f}f, {n[2]:.4f}f, {len(loop)}, {{ {', '.join(map(str, loop))} }} }},")
	out += ["};", "", f"static const WireEdge_t _{name}_edges[{len(edges)}] = {{"]
	out += [f"\t{{ {a}, {b}, {f0}, {f1}, {c} }}," for a, b, f0, f1, c in edges]
	out += ["};", ""]
	if polys:
		out += [f"static const WirePoly_t _{name}_polys[{len(polys)}] = {{"]
		out += [f"\t{{ {f}, {c}, {len(idx)}, {{ {', '.join(map(str, idx))} }} }}," for f, c, idx in polys]
		out += ["};", ""]
	out += [
		f"const WireModel_t model_{name} = {{",
		f"\t_{name}_vertices, _{name}_faces, _{name}_edges, {f'_{name}_polys' if polys else 'NULL'},",
		f"\t{len(verts)}, {len(hull)}, {len(faces)}, {len(edges)}, {len(polys)}, {radius:.1f}f,",
		f'\t"{m["name"]}", {{ {m["rgb"][0]}, {m["rgb"][1]}, {m["rgb"][2]} }}',
		"};",
		"",
	]
	print(f"{name}: {len(verts)} corners, {len(faces)} faces, {len(edges)} edges, {len(polys)} shapes")


def main():
	out = [
		"// Generated by art/ship_model.py, edit that and run it again",
		'#include "ship_model.h"',
		"",
	]
	for name, m in MODELS.items():
		emit_model(out, name, m)
	out += [f'_Static_assert(MODEL_SHIP_COUNT == {len(SHIPS)}, "MODEL_SHIP_COUNT in ship_model.h");', ""]
	out += ["const WireModel_t* const model_ships[MODEL_SHIP_COUNT] = {"]
	out += [f"\t&model_{name}," for name in SHIPS]
	out += ["};", ""]
	path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "ship_model.c")
	with open(path, "w") as f:
		f.write("\n".join(out))


if __name__ == "__main__":
	main()
