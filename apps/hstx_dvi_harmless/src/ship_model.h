#pragma once

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int16_t x, y, z;
} WireVertex_t;

// The most corners on a face or a shape
#define WIRE_MAX_LOOP 8

typedef struct {
	float nx, ny, nz; // pointing out
	uint8_t n;
	uint8_t v[WIRE_MAX_LOOP]; // the corners in order round the face
} WireFace_t;

// An edge is drawn if either of the faces it joins is facing us
typedef struct {
	uint8_t v0, v1;
	uint8_t f0, f1;
	uint8_t colour;
} WireEdge_t;

// A shape drawn over a face, such as an engine
typedef struct {
	uint8_t f;
	uint8_t colour;
	uint8_t n;
	uint8_t v[WIRE_MAX_LOOP];
} WirePoly_t;

typedef struct {
	const WireVertex_t* v;
	const WireFace_t* f;
	const WireEdge_t* e;
	const WirePoly_t* p;
	uint8_t nv;
	uint8_t nh; // the first nh corners are the hull, the rest are details
	uint8_t nf, ne, np;
	float radius;
	const char* name;
	uint8_t rgb[3];
} WireModel_t;

extern const WireModel_t model_cobra;
extern const WireModel_t model_coriolis;
extern const WireModel_t model_dodo;
extern const WireModel_t model_missile;
extern const WireModel_t model_canister;

// The ships that fly round the station
#define MODEL_SHIP_COUNT 24
extern const WireModel_t* const model_ships[MODEL_SHIP_COUNT];

#ifdef __cplusplus
}
#endif
