#!/bin/sh
# Builds the wireframe test app to run on the PC, for trying changes without a Pico.
#
# host/build.sh [build dir]
H=$(cd "$(dirname "$0")" && pwd)
exec "$H/../../common/host/build.sh" "$H/.." wireframe_host "$@"
