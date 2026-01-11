#include "../../src/fsdb_plugin_api.h"

#include <stdlib.h>
#include <string.h>

static int fsdb_stub_convert_to_fst(const char *fsdb_path,
                                    const char *fst_path,
                                    char **error_message)
{
    const char *msg = "FSDB plugin stub: no conversion implemented";
    size_t len = strlen(msg) + 1;
    char *copy = (char *)malloc(len);

    (void)fsdb_path;
    (void)fst_path;

    if (copy) {
        memcpy(copy, msg, len);
    }

    if (error_message) {
        *error_message = copy;
    } else {
        free(copy);
    }

    return 1;
}

static void fsdb_stub_free_error(char *error_message)
{
    free(error_message);
}

static const GtkwaveFsdbPluginInfo fsdb_stub_info = {
    GTKWAVE_FSDB_PLUGIN_API_VERSION,
    "fsdb-stub",
    "0.1",
    "gtkwave",
};

static const GtkwaveFsdbPluginApi fsdb_stub_api = {
    GTKWAVE_FSDB_PLUGIN_API_VERSION,
    &fsdb_stub_info,
    fsdb_stub_convert_to_fst,
    fsdb_stub_free_error,
};

const GtkwaveFsdbPluginApi *gtkwave_fsdb_plugin_get_api(void)
{
    return &fsdb_stub_api;
}
