#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int16_t x, y, z;
} WireVertex_t;

// A face is only needed to say which way it points
typedef struct {
	float nx, ny, nz;
	uint8_t v; // a corner on the face
} WireFace_t;

// An edge is drawn if either of the faces it joins is facing us
typedef struct {
	uint8_t v0, v1;
	uint8_t f0, f1;
	uint8_t colour;
} WireEdge_t;

typedef struct {
	const WireVertex_t* v;
	const WireFace_t* f;
	const WireEdge_t* e;
	uint8_t nv, nf, ne;
} WireModel_t;

extern const WireModel_t ship_cobra;

#ifdef __cplusplus
}
#endif
