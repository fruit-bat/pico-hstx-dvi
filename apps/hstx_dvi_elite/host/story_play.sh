#!/bin/sh
# Builds the story player and runs it, see story_play.c.
#
# host/story_play.sh [build dir]
set -e
G=$(cd "$(dirname "$0")/.." && pwd)
B=${1:-$G/host/build}
mkdir -p "$B"
gcc -O1 -g -std=gnu11 -Wall -Wextra -Wno-missing-field-initializers -I"$G/src" -o "$B/story_play" \
	"$G/host/story_play.c" "$G/src/el_story.c" "$G/src/el_story_data.c" "$G/src/el_companion.c" "$G/src/galaxy.c" -lm
"$B/story_play"
