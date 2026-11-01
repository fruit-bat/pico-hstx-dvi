// Drives the Elite game on the PC, see common/host
//
// attract - nobody touches the buttons
// fly     - starts a game, speeds up, rolls and climbs, looks at the charts
//           moving the cursor, then changes the style in the options
#include <stdio.h>
#include <string.h>
#include "host.h"

// The buttons, as in el_input.h
#define L 1
#define R 2
#define U 4
#define D 8
#define A 16
#define B 32
#define SELECT 64
#define START 128

uint32_t host_input(uint32_t f) {
	if (!strcmp(host_mode, "fly")) {
		if (f >= 30 && f < 33) return START;
		// Speed up, then roll right and climb
		if (f >= 60 && f < 160) return B | U;
		if (f >= 200 && f < 260) return R;
		if (f >= 300 && f < 380) return D;
		// The charts, moving the cursor
		if (f >= 500 && f < 503) return SELECT;
		if (f >= 520 && f < 600) return R | D;
		if (f >= 700 && f < 703) return SELECT;
		if (f >= 720 && f < 800) return L;
		if (f >= 900 && f < 903) return SELECT;
		if (f >= 1100 && f < 1103) return SELECT;
		// The options: filled and dithered
		if (f >= 1200 && f < 1203) return START;
		if (f >= 1220 && f < 1223) return R;
		if (f >= 1230 && f < 1233) return R;
		if (f >= 1240 && f < 1243) return R;
		if (f >= 1250 && f < 1253) return D;
		if (f >= 1260 && f < 1263) return R;
		if (f >= 1300 && f < 1303) return START;
		// Dive
		if (f >= 1400 && f < 1460) return U;
	}
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (frame % 600 == 0) {
		printf("%6u sprites %3u\n", frame, sprites);
	}
}
