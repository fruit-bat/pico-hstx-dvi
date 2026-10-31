#!/usr/bin/env python3
# Generates the android's walk cycle and flowing hair as ASCII art in
# ../src/android_art.c. The body is posed from a simple skeleton for each
# frame, so the feet stay on the ground and the cycle loops smoothly.
#
# python3 art/android_art.py

import math
import os

W = 32
BODY_H = 48
HAIR_H = 32
WALK_FRAMES = 8
HAIR_FRAMES = 8

# Body layers, one character each. The layers never overlap within a frame.
#   C glow, S skin, A armour, B suit, F the far limbs, O outline
BODY_KEY = "CSABFO"
# Hair layers, drawn over the body: H highlight, h hair
HAIR_KEY = "Hh"

# The skeleton, facing right, before the body is lowered to keep a foot on the ground
CX = 19
HEAD = (19.5, 6.5)
HEAD_RX = 3.6
HEAD_RY = 4.4
SHOULDER = (19.0, 13.5)
HIP = (19.0, 24.5)
THIGH = 11.0
SHIN = 11.0
UPPER_ARM = 6.5
FOREARM = 6.0
GROUND = 47


class Canvas:
	def __init__(self, h):
		self.h = h
		self.c = [['.'] * W for _ in range(h)]

	def get(self, x, y):
		if 0 <= x < W and 0 <= y < self.h:
			return self.c[y][x]
		return '.'

	def set(self, x, y, ch):
		if 0 <= x < W and 0 <= y < self.h:
			self.c[y][x] = ch

	def disc(self, cx, cy, r, ch):
		for y in range(int(cy - r) - 1, int(cy + r) + 2):
			for x in range(int(cx - r) - 1, int(cx + r) + 2):
				if (x + 0.5 - cx) ** 2 + (y + 0.5 - cy) ** 2 <= r * r:
					self.set(x, y, ch)

	def segment(self, a, b, r, ch):
		n = int(max(abs(b[0] - a[0]), abs(b[1] - a[1])) * 4) + 1
		for i in range(n + 1):
			t = i / n
			self.disc(a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, r, ch)

	def line(self, a, b, ch, only=None):
		n = int(max(abs(b[0] - a[0]), abs(b[1] - a[1])) * 2) + 1
		for i in range(n + 1):
			t = i / n
			x = int(a[0] + (b[0] - a[0]) * t)
			y = int(a[1] + (b[1] - a[1]) * t)
			if only is None or self.get(x, y) in only:
				self.set(x, y, ch)

	def outline(self, ch):
		edge = []
		for y in range(self.h):
			for x in range(W):
				if self.c[y][x] != '.':
					continue
				for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
					if self.get(x + dx, y + dy) not in '.' + ch:
						edge.append((x, y))
						break
		for x, y in edge:
			self.set(x, y, ch)

	def rows(self):
		return [''.join(r) for r in self.c]


def limb(root, a1, l1, a2, l2):
	# Angles from straight down, positive forwards
	j = (root[0] + l1 * math.sin(a1), root[1] + l1 * math.cos(a1))
	e = (j[0] + l2 * math.sin(a2), j[1] + l2 * math.cos(a2))
	return j, e


def leg_angles(t):
	thigh = 0.42 * math.cos(t)
	# The knee bends as the leg swings forwards
	knee = 0.15 + 0.8 * max(0.0, -math.sin(t))
	return thigh, thigh - knee


def arm_angles(t):
	upper = -0.45 * math.cos(t)
	return upper, upper + 0.35 + 0.2 * max(0.0, -math.cos(t))


def pose(p):
	t = 2 * math.pi * p / WALK_FRAMES
	legs = []
	for lt in (t, t + math.pi):
		a1, a2 = leg_angles(lt)
		legs.append(limb(HIP, a1, THIGH, a2, SHIN))
	# Lower the body until the lowest foot is on the ground
	drop = int(GROUND - 1 - max(int(l[1][1]) for l in legs))
	return t, legs, drop


def draw_leg(c, knee, ankle, near, dy):
	hip = (HIP[0], HIP[1] + dy)
	knee = (knee[0], knee[1] + dy)
	ankle = (ankle[0], ankle[1] + dy)
	suit = 'B' if near else 'F'
	boot = 'A' if near else 'F'
	c.segment(hip, knee, 1.6, suit)
	c.segment(knee, ankle, 1.2, suit)
	# Boots up to half way up the shin
	mid = ((knee[0] + ankle[0]) / 2, (knee[1] + ankle[1]) / 2)
	c.segment(mid, ankle, 1.3, boot)
	# The foot is square to the shin, so it points its toe as the leg lifts
	a2 = math.atan2(ankle[0] - knee[0], ankle[1] - knee[1])
	fx, fy = math.cos(a2), -math.sin(a2)
	if fy < 0:
		fx, fy = 1.0, 0.0
	c.segment((ankle[0] - fx, ankle[1] - fy), (ankle[0] + 3.0 * fx, ankle[1] + 3.0 * fy), 0.8, boot)
	if near:
		c.disc(knee[0], knee[1], 1.2, 'A')
		# A light down the side of the leg
		c.line(hip, (knee[0], knee[1] - 1), 'C', only='B')


def draw_arm(c, t, near, dy):
	a1, a2 = arm_angles(t)
	sh = (SHOULDER[0], SHOULDER[1] + dy)
	elbow, wrist = limb(sh, a1, UPPER_ARM, a2, FOREARM)
	suit = 'B' if near else 'F'
	c.segment(sh, elbow, 1.2, suit)
	c.segment(elbow, wrist, 1.0, suit)
	c.disc(wrist[0], wrist[1], 1.1, 'S' if near else 'F')
	if near:
		# Armoured shoulder
		c.disc(sh[0] - 0.3, sh[1] - 0.2, 2.0, 'A')


def draw_body(c, dy):
	# Neck
	c.segment((19.5, 10.0 + dy), (19.5, 12.5 + dy), 1.0, 'S')
	# Torso, a little narrower at the waist
	for y in range(12, 25):
		yy = y + dy
		if y < 18:
			x0, x1 = 16.2, 22.4 + (0.6 if 14 <= y <= 16 else 0.0)
		elif y < 21:
			x0, x1 = 16.8, 21.6
		else:
			x0, x1 = 16.2, 22.2
		for x in range(int(x0), int(math.ceil(x1))):
			c.set(x, yy, 'B')
	# Belt and chest light
	for x in range(16, 23):
		c.set(x, 21 + dy, 'A')
	c.set(21, 15 + dy, 'C')
	c.set(22, 15 + dy, 'C')


def draw_head(c, dy):
	hx, hy = HEAD[0], HEAD[1] + dy
	for y in range(int(hy - HEAD_RY) - 1, int(hy + HEAD_RY) + 2):
		for x in range(int(hx - HEAD_RX) - 1, int(hx + HEAD_RX) + 2):
			if ((x + 0.5 - hx) / HEAD_RX) ** 2 + ((y + 0.5 - hy) / HEAD_RY) ** 2 <= 1.0:
				c.set(x, y, 'S')
	# A little nose and chin
	c.set(23, int(hy), 'S')
	# A glowing eye
	c.set(21, int(hy - 0.5), 'C')


def body_frame(p):
	t, legs, dy = pose(p)
	c = Canvas(BODY_H)
	# Back to front: far arm and leg, body, near leg and arm
	draw_arm(c, t + math.pi, False, dy)
	draw_leg(c, legs[1][0], legs[1][1], False, dy)
	draw_body(c, dy)
	draw_head(c, dy)
	draw_leg(c, legs[0][0], legs[0][1], True, dy)
	draw_arm(c, t, True, dy)
	c.outline('O')
	return c.rows(), dy


def hair_frame(p):
	ph = 2 * math.pi * p / HAIR_FRAMES
	c = Canvas(HAIR_H)
	hx, hy = HEAD
	# The cap over the top and back of the head
	for y in range(0, 12):
		for x in range(12, 25):
			d = ((x + 0.5 - hx) / (HEAD_RX + 1.0)) ** 2 + ((y + 0.5 - hy) / (HEAD_RY + 1.0)) ** 2
			front = x + 0.5 - hx
			if d <= 1.0 and (y < 4 or front < -0.5 or (y < 5 and front < 2.5)):
				c.set(x, y, 'h')
	# A fringe over the forehead
	c.set(22, 3, 'h')
	c.set(23, 3, 'h')
	c.set(22, 4, 'h')
	# Strands streaming out behind, rippling as she walks
	strands = 6
	for i in range(strands):
		f = i / (strands - 1)
		root = (17.0 - 1.5 * f, 2.5 + 7.0 * f)
		length = 11.0 + 2.0 * math.sin(i * 1.7) + 2.0 * f
		droop = 0.45 + 0.6 * f
		dx, dy = -math.cos(droop), math.sin(droop)
		pts = []
		n = int(length * 2)
		for k in range(n + 1):
			s = length * k / n
			wave = 0.11 * s * math.sin(2 * math.pi * s / 10.0 - ph + i * 0.8)
			pts.append((root[0] + dx * s - dy * wave, root[1] + dy * s + dx * wave))
		for k in range(n):
			r = 1.05 - 0.2 * k / n
			c.segment(pts[k], pts[k + 1], r, 'h')
		# Light catches the upper strands
		if i in (1, 3):
			for k in range(1, n // 2):
				x, y = int(pts[k][0]), int(pts[k][1])
				if c.get(x, y) == 'h':
					c.set(x, y, 'H')
	# A shine across the top of the head
	for x in range(17, 22):
		if c.get(x, 2) == 'h':
			c.set(x, 2, 'H')
	return c.rows()


def emit_frames(out, name, frames, h):
	out.append(f"static const char* const {name}[{len(frames)}][{h}] = {{")
	for rows in frames:
		out.append("\t{")
		for r in rows:
			out.append(f'\t\t"{r}",')
		out.append("\t},")
	out.append("};")
	out.append("")


def main():
	body = [body_frame(p) for p in range(WALK_FRAMES)]
	hair = [hair_frame(p) for p in range(HAIR_FRAMES)]

	# How far the body moves on from one frame to the next, from the foot on the ground
	steps = []
	for p in range(WALK_FRAMES):
		_, legs0, _ = pose(p)
		_, legs1, _ = pose(p + 1)
		# The near foot is on the ground while the far leg swings, see leg_angles
		down = 0 if math.sin(2 * math.pi * (p + 0.5) / WALK_FRAMES) >= 0 else 1
		steps.append(legs0[down][1][0] - legs1[down][1][0])

	out = [
		"// Generated by art/android_art.py, edit that and run it again",
		'#include "android_art.h"',
		"",
		f'const char* const android_body_key = "{BODY_KEY}";',
		f'const char* const android_hair_key = "{HAIR_KEY}";',
		"",
		"// The middle of the body, for turning round",
		f"const int32_t android_cx = {CX};",
		"",
		"// How far the body is lowered in each frame, to keep a foot on the ground",
		"const int8_t android_bob[ANDROID_WALK_FRAMES] = { " + ", ".join(str(int(b[1])) for b in body) + " };",
		"",
		"// How far she moves on after each frame, in 1/256ths of a pixel",
		"const int16_t android_step[ANDROID_WALK_FRAMES] = { " + ", ".join(str(round(s * 256)) for s in steps) + " };",
		"",
	]
	emit_frames(out, "_android_body", [b[0] for b in body], BODY_H)
	emit_frames(out, "_android_hair", hair, HAIR_H)
	out += [
		"const char* const* android_body_rows(uint32_t frame) {",
		"\treturn _android_body[frame % ANDROID_WALK_FRAMES];",
		"}",
		"",
		"const char* const* android_hair_rows(uint32_t frame) {",
		"\treturn _android_hair[frame % ANDROID_HAIR_FRAMES];",
		"}",
	]
	path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "android_art.c")
	with open(path, "w") as f:
		f.write("\n".join(out) + "\n")


if __name__ == "__main__":
	main()
