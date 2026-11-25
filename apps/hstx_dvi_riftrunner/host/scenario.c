// Drives the rift runner app on the PC, see common/host
//
// attract  - nobody touches the buttons
// play     - starts a game and lets the demo pilot fly it
// tour     - as play, jumping to each section in turn, every 600 frames
// boss     - as play, going straight to the warship
// manual   - starts a game, charges the beam and lets it go, bombs, takes a power up
// settings - opens the settings on the title, plays a sound and goes back
#include <stdio.h>
#include <string.h>
#include "host.h"

// From the game, to drive and report on it
uint16_t rr_demo_input(uint32_t frame);
uint32_t rr_hud_score(void);
uint32_t rr_hud_lives(void);
int32_t rr_player_fuel(void);
int32_t rr_player_py(void);
uint32_t rr_enemy_fliers(void);
int32_t rr_world_camera(void);
uint32_t rr_world_section(int32_t px);
uint32_t rr_world_loop(void);
void rr_world_jump(uint32_t section);
bool rr_boss_active(void);
bool rr_player_alive(void);

#define F 16
#define START 64

static bool flying(void) {
	return !strcmp(host_mode, "play") || !strcmp(host_mode, "tour") || !strcmp(host_mode, "boss");
}

// Buttons as the game sees them: L R U D F B start power from bit 0
uint32_t host_input(uint32_t f) {
	if (flying()) {
		if (f > 100 && f < 104) return F;
		if (f > 120) return rr_demo_input(f);
	}
	if (!strcmp(host_mode, "manual")) {
		if (f > 100 && f < 104) return F;
		// Hold fire to charge, let go, drop bombs, then up and down
		if (f > 200 && f < 320) return F;
		if (f == 400 || f == 440) return 32;
		if (f > 460 && f < 500) return 4;
		if (f > 520 && f < 560) return 8 | 2;
		if ((f > 600 && f < 900) && (f & 8)) return F;
	}
	if (!strcmp(host_mode, "settings")) {
		// One frame each: start, down to the sounds, right twice, fire, start
		static const uint16_t at[][2] = {
			{ 100, START }, { 160, 8 }, { 200, 2 }, { 220, 2 }, { 240, F }, { 330, START },
		};
		for (uint32_t i = 0; i < sizeof(at) / sizeof(at[0]); ++i) if (f == at[i][0]) return at[i][1];
	}
	return 0;
}

void host_frame(uint32_t frame, uint32_t sprites) {
	static uint32_t most = 0;
	if (sprites > most) most = sprites;
	if (!strcmp(host_mode, "tour") && frame > 120 && frame % 600 == 150) {
		rr_world_jump((frame / 600) % 6);
	}
	if (!strcmp(host_mode, "boss") && frame == 130) rr_world_jump(5);
	static int32_t lives = -1;
	if (lives >= 0 && rr_hud_lives() != (uint32_t)lives) printf("%6u lives %u\n", frame, rr_hud_lives());
	lives = rr_hud_lives();
	static int32_t alive = 1;
	if (alive && !rr_player_alive()) printf("%6u died at y %d\n", frame, rr_player_py());
	alive = rr_player_alive();
	if (frame % 600 == 0) {
		const int32_t cam = rr_world_camera() >> 8;
		printf("%6u score %7u lives %u fuel %4d section %u-%u fliers %2u boss %d sprites %3u (most %3u)\n",
			frame, rr_hud_score(), rr_hud_lives(), rr_player_fuel(), rr_world_loop() + 1, rr_world_section(cam + 320) + 1,
			rr_enemy_fliers(), rr_boss_active(), sprites, most);
		most = 0;
	}
}
