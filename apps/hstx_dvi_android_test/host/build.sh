#!/bin/sh
# Builds the android test app to run on the PC, for trying changes without a Pico.
#
# host/build.sh [build dir]
H=$(cd "$(dirname "$0")" && pwd)
exec "$H/../../common/host/build.sh" "$H/.." android_host "$@"
