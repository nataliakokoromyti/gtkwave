#!/usr/bin/env sh
set -eu

gtkwave_bin="${1:-./build/src/gtkwave}"
wave_file="${2:-./examples/des.fst}"

if ! command -v broadwayd >/dev/null 2>&1; then
    echo "error: broadwayd not found in PATH" >&2
    exit 1
fi

if [ ! -x "$gtkwave_bin" ]; then
    echo "error: gtkwave not found at $gtkwave_bin" >&2
    exit 1
fi

display=":5"
url="http://127.0.0.1:8085/"

cleanup() {
    if [ -n "${gtkwave_pid:-}" ] && kill -0 "$gtkwave_pid" 2>/dev/null; then
        kill "$gtkwave_pid" 2>/dev/null || true
    fi
    if [ -n "${broadway_pid:-}" ] && kill -0 "$broadway_pid" 2>/dev/null; then
        kill "$broadway_pid" 2>/dev/null || true
    fi
}

trap cleanup EXIT INT TERM

broadwayd "$display" >/tmp/broadwayd.log 2>&1 &
broadway_pid=$!

sleep 1

echo "Broadway server running at $url"
echo "Launching GTKWave with Broadway backend..."

GDK_BACKEND=broadway BROADWAY_DISPLAY="$display" "$gtkwave_bin" "$wave_file" &
gtkwave_pid=$!

wait "$gtkwave_pid"
