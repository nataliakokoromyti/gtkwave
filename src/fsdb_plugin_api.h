#ifndef GTKWAVE_FSDB_PLUGIN_API_H
#define GTKWAVE_FSDB_PLUGIN_API_H

#include <stdint.h>

#define GTKWAVE_FSDB_PLUGIN_API_VERSION 1
#define GTKWAVE_FSDB_PLUGIN_GET_API_SYMBOL "gtkwave_fsdb_plugin_get_api"

typedef struct GtkwaveFsdbPluginInfo {
    uint32_t api_version;
    const char *name;
    const char *version;
    const char *vendor;
} GtkwaveFsdbPluginInfo;

typedef struct GtkwaveFsdbPluginApi {
    uint32_t api_version;
    const GtkwaveFsdbPluginInfo *info;
    int (*convert_to_fst)(const char *fsdb_path, const char *fst_path, char **error_message);
    void (*free_error)(char *error_message);
} GtkwaveFsdbPluginApi;

typedef const GtkwaveFsdbPluginApi *(*GtkwaveFsdbPluginGetApiFunc)(void);

#endif
