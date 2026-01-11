#include "fsdb_plugin.h"

#include "debug.h"

#include <glib.h>
#include <gmodule.h>
#include <stdarg.h>
#include <stdlib.h>

#ifdef __MINGW32__
#include <io.h>
#define close _close
#define unlink _unlink
#else
#include <unistd.h>
#endif

static void fsdb_plugin_set_error(char **error_message, const char *format, ...)
{
    va_list args;

    if (!error_message) {
        return;
    }

    va_start(args, format);
    *error_message = g_strdup_vprintf(format, args);
    va_end(args);
}

static void fsdb_plugin_free_error(const GtkwaveFsdbPluginApi *api, char *error_message)
{
    if (!error_message) {
        return;
    }

    if (api && api->free_error) {
        api->free_error(error_message);
    } else {
        free(error_message);
    }
}

char *fsdb_plugin_convert_to_fst(const char *fsdb_path,
                                 const char *plugin_path,
                                 char **error_message)
{
    GModule *module = NULL;
    GtkwaveFsdbPluginGetApiFunc get_api = NULL;
    const GtkwaveFsdbPluginApi *api = NULL;
    char *output_path = NULL;
    char *plugin_error = NULL;
    int fd = -1;
    int rc = 0;

    if (!fsdb_path || !*fsdb_path) {
        fsdb_plugin_set_error(error_message, "Missing FSDB input path");
        return NULL;
    }

    if (!plugin_path || !*plugin_path) {
        fsdb_plugin_set_error(error_message, "FSDB plugin path not configured");
        return NULL;
    }

    if (!g_module_supported()) {
        fsdb_plugin_set_error(error_message, "Dynamic modules are not supported on this platform");
        return NULL;
    }

    module = g_module_open(plugin_path, G_MODULE_BIND_LAZY);
    if (!module) {
        fsdb_plugin_set_error(error_message, "Failed to load FSDB plugin: %s", g_module_error());
        return NULL;
    }

    if (!g_module_symbol(module,
                         GTKWAVE_FSDB_PLUGIN_GET_API_SYMBOL,
                         (gpointer *)&get_api) ||
        !get_api) {
        fsdb_plugin_set_error(error_message,
                              "FSDB plugin missing %s symbol",
                              GTKWAVE_FSDB_PLUGIN_GET_API_SYMBOL);
        g_module_close(module);
        return NULL;
    }

    api = get_api();
    if (!api || api->api_version != GTKWAVE_FSDB_PLUGIN_API_VERSION || !api->convert_to_fst) {
        fsdb_plugin_set_error(error_message, "FSDB plugin API version mismatch");
        g_module_close(module);
        return NULL;
    }

    output_path = tmpnam_2(NULL, &fd);
    if (!output_path) {
        fsdb_plugin_set_error(error_message, "Failed to create temporary output path");
        g_module_close(module);
        return NULL;
    }

    if (fd >= 0) {
        close(fd);
    }

    rc = api->convert_to_fst(fsdb_path, output_path, &plugin_error);
    if (rc != 0) {
        if (plugin_error) {
            fsdb_plugin_set_error(error_message, "%s", plugin_error);
        } else {
            fsdb_plugin_set_error(error_message, "FSDB plugin conversion failed");
        }
        fsdb_plugin_free_error(api, plugin_error);
        unlink(output_path);
        free_2(output_path);
        g_module_close(module);
        return NULL;
    }

    fsdb_plugin_free_error(api, plugin_error);
    g_module_close(module);
    return output_path;
}

void fsdb_plugin_cleanup_temp_file(const char *path)
{
    if (!path || !*path) {
        return;
    }

    unlink(path);
}
