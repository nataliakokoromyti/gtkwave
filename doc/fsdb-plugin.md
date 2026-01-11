FSDB Plugin Support
===================

GTKWave can load FSDB files through a runtime plugin that converts FSDB to FST.
The plugin is not bundled because the FSDB SDK is proprietary.

Usage
-----

Provide the plugin path using either:

    gtkwave --fsdb-plugin=/path/to/libgtkwave_fsdb_plugin.so mydump.fsdb

or:

    export GTKWAVE_FSDB_PLUGIN=/path/to/libgtkwave_fsdb_plugin.so
    gtkwave mydump.fsdb

Build the plugin from the FSDB SDK (from the repo root):

    FSDB_SDK_DIR=/path/to/FsdbReader \
      contrib/fsdb-plugin/build-fsdb-plugin.sh ./build/fsdb-plugin

Then run:

    gtkwave --fsdb-plugin=./build/fsdb-plugin/libgtkwave_fsdb_plugin.so mydump.fsdb

Plugin API
----------

Implement the API defined in `src/fsdb_plugin_api.h` and export the symbol:

    gtkwave_fsdb_plugin_get_api

The plugin must provide `convert_to_fst(fsdb_path, fst_path, error_message)`.
GTKWave will call the plugin, then load the generated FST.

If the plugin relies on external helpers, it can use:

- `GTKWAVE_FSDB2VCD` to locate `fsdb2vcd_fast`
- `GTKWAVE_VCD2FST` to locate `vcd2fst`

Example
-------

See:

- `contrib/fsdb-plugin/fsdb_plugin_stub.c` for a minimal stub
- `contrib/fsdb-plugin/fsdb_plugin_external.c` for a helper-based plugin
