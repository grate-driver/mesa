#!/bin/sh
set -eu
: "${CC:?Set CC to the target compiler executable; pass sysroot/include/link flags as arguments}"
task_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
"$CC" "$@" -std=c99 -O2 -g -Wall -Wextra -Werror "$task_dir/blend-probe.c" -lGL -lX11 -lm -o "${GRATE_BLEND_OUTPUT:-$task_dir/blend-probe}"
