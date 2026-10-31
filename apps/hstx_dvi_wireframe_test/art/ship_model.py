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

# Each model has the corners of its hull, x right, y up, z forwards, and its
# details: (name, corners, a shape or just lines?, colour)
MODELS = {
	# As in the original game
	"cobra": {
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
		"hull": [
			(s * 160, t * 160, 0) for s in (1, -1) for t in (1, -1)
		] + [
			(s * 160, 0, t * 160) for s in (1, -1) for t in (1, -1)
		] + [
			(0, s * 160, t * 160) for s in (1, -1) for t in (1, -1)
		],
		"details": [
			("slot", [(-40, 12, 160), (40, 12, 160), (40, -12, 160), (-40, -12, 160)], True, TRIM),
		],
	},
	# A pointed body with four fins at the back
	"missile": {
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
}

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


def emit_model(out, name, hull, details):
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
		f"\t{len(verts)}, {len(hull)}, {len(faces)}, {len(edges)}, {len(polys)}, {radius:.1f}f",
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
		emit_model(out, name, m["hull"], m["details"])
	path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "ship_model.c")
	with open(path, "w") as f:
		f.write("\n".join(out))


if __name__ == "__main__":
	main()
