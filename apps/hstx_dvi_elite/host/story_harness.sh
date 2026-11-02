#!/bin/sh
# Builds the story harness and runs it, see story_harness.c.
#
# host/story_harness.sh [build dir] [story number]
set -e
G=$(cd "$(dirname "$0")/.." && pwd)
B=${1:-$G/host/build}
mkdir -p "$B"
gcc -O1 -g -std=gnu11 -Wall -Wextra -Wno-missing-field-initializers -I"$G/src" -o "$B/story_harness" \
	"$G/host/story_harness.c" "$G/src/el_story.c" "$G/src/el_story_data.c" "$G/src/el_companion.c" "$G/src/galaxy.c" -lm
"$B/story_harness" $2
