#!/usr/bin/env sh
set -eu

out_dir="${1:-.}"
plugin_name="libgtkwave_fsdb_plugin.so"
fsdb2vcd_name="fsdb2vcd_fast"

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root_dir=$(CDPATH= cd -- "$script_dir/../.." && pwd)

if [ -z "${FSDB_SDK_DIR:-}" ] && { [ -z "${FSDBREADER_INCLUDE:-}" ] || [ -z "${FSDBREADER_LIB:-}" ]; }; then
    echo "error: set FSDB_SDK_DIR or FSDBREADER_INCLUDE/FSDBREADER_LIB" >&2
    exit 1
fi

if [ -n "${FSDB_SDK_DIR:-}" ]; then
    FSDBREADER_INCLUDE="${FSDBREADER_INCLUDE:-$FSDB_SDK_DIR}"
    FSDBREADER_LIB="${FSDBREADER_LIB:-$FSDB_SDK_DIR}"
fi

mkdir -p "$out_dir"

cxx="${CXX:-c++}"
cc="${CC:-cc}"

fsdb2vcd_src="$root_dir/contrib/fsdb2vcd/fsdb2vcd_fast.cc"
plugin_src="$root_dir/contrib/fsdb-plugin/fsdb_plugin_external.c"

"$cxx" -O2 -o "$out_dir/$fsdb2vcd_name" "$fsdb2vcd_src" \
    -I "$FSDBREADER_INCLUDE" \
    "$FSDBREADER_LIB/libnffr.a" "$FSDBREADER_LIB/libnsys.a" \
    -ldl -lpthread -lz

"$cc" -shared -fPIC -o "$out_dir/$plugin_name" "$plugin_src"

echo "Built:"
echo "  $out_dir/$plugin_name"
echo "  $out_dir/$fsdb2vcd_name"
