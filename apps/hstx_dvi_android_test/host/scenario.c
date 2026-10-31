// Drives the android test app on the PC, see common/host
//
// attract - she just walks up and down
#include <stdio.h>
#include "host.h"

uint32_t host_input(uint32_t f) {
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	if (frame % 600 == 0) {
		printf("%6u sprites %3u\n", frame, sprites);
	}
}
