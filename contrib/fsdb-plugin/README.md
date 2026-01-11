FSDB Plugin Examples
====================

This directory contains two example plugins:

- `fsdb_plugin_stub.c` returns a fixed error (loader sanity check).
- `fsdb_plugin_external.c` shells out to `fsdb2vcd_fast` and `vcd2fst`.

Build stub (Linux)
------------------

    cc -shared -fPIC -o libgtkwave_fsdb_plugin_stub.so fsdb_plugin_stub.c

Then run GTKWave with:

    gtkwave --fsdb-plugin=/path/to/libgtkwave_fsdb_plugin_stub.so file.fsdb

Build external plugin (Linux)
-----------------------------

From the repo root:

    FSDB_SDK_DIR=/path/to/FsdbReader \
      contrib/fsdb-plugin/build-fsdb-plugin.sh ./build/fsdb-plugin

Then run:

    gtkwave --fsdb-plugin=./build/fsdb-plugin/libgtkwave_fsdb_plugin.so file.fsdb
