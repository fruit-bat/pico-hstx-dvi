#!/bin/sh
# Builds the manor app to run on the PC, for trying changes without a Pico.
#
# host/build.sh [build dir]
H=$(cd "$(dirname "$0")" && pwd)
exec "$H/../../common/host/build.sh" "$H/.." manor_host "$@"
