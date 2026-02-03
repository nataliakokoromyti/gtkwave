#include "../../src/fsdb_plugin_api.h"

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

static int fsdb_run_cmd(char **argv, char **error_message)
{
    GError *error = NULL;
    gchar *std_error = NULL;
    gint exit_status = 0;
    gboolean ok;

    ok = g_spawn_sync(NULL,              /* working directory */
                      argv,              /* argument vector */
                      NULL,              /* environment */
                      0,                 /* flags */
                      NULL,              /* child setup func */
                      NULL,              /* user data */
                      NULL,              /* stdout */
                      &std_error,        /* stderr */
                      &exit_status,      /* exit status */
                      &error);           /* error */

    if (!ok) {
        if (error_message) {
            *error_message = g_strdup_printf("Failed to execute %s: %s",
                                             argv[0],
                                             error ? error->message : "unknown error");
        }
        if (error) {
            g_error_free(error);
        }
        g_free(std_error);
        return 0;
    }

    /* Check exit status - handles both exit codes and signals */
    if (!g_spawn_check_exit_status(exit_status, &error)) {
        if (error_message) {
            if (std_error && *std_error) {
                *error_message = g_strdup_printf("%s failed: %s\nStderr: %s",
                                                 argv[0],
                                                 error->message,
                                                 std_error);
            } else {
                *error_message = g_strdup_printf("%s failed: %s",
                                                 argv[0],
                                                 error->message);
            }
        }
        if (error) {
            g_error_free(error);
        }
        g_free(std_error);
        return 0;
    }

    g_free(std_error);
    return 1;
}

static char *fsdb_make_temp(char **error_message)
{
    GError *error = NULL;
    char *filename = NULL;
    gint fd = -1;

    fd = g_file_open_tmp("gtkwave-fsdb-XXXXXX.vcd", &filename, &error);
    if (fd < 0) {
        if (error_message && error) {
            *error_message = g_strdup_printf("Failed to create temp file: %s",
                                             error->message);
        }
        if (error) {
            g_error_free(error);
        }
        return NULL;
    }

    close(fd);
    return filename;
}

static int fsdb_external_convert_to_fst(const char *fsdb_path,
                                        const char *fst_path,
                                        char **error_message)
{
    const char *fsdb2vcd = getenv("GTKWAVE_FSDB2VCD");
    const char *vcd2fst = getenv("GTKWAVE_VCD2FST");
    char *fsdb2vcd_path = NULL;
    char *vcd2fst_path = NULL;
    char *tmp_vcd = NULL;
    char *argv[6];
    int ok = 0;

    if (!fsdb2vcd || !*fsdb2vcd) {
        fsdb2vcd = "fsdb2vcd_fast";
    }
    if (!vcd2fst || !*vcd2fst) {
        vcd2fst = "vcd2fst";
    }

    /* Check if tools exist in PATH */
    fsdb2vcd_path = g_find_program_in_path(fsdb2vcd);
    if (!fsdb2vcd_path) {
        if (error_message) {
            *error_message = g_strdup_printf("Tool not found in PATH: %s\n"
                                             "Set GTKWAVE_FSDB2VCD to full path or add to PATH",
                                             fsdb2vcd);
        }
        return 1;
    }

    vcd2fst_path = g_find_program_in_path(vcd2fst);
    if (!vcd2fst_path) {
        if (error_message) {
            *error_message = g_strdup_printf("Tool not found in PATH: %s\n"
                                             "Set GTKWAVE_VCD2FST to full path or add to PATH",
                                             vcd2fst);
        }
        g_free(fsdb2vcd_path);
        return 1;
    }

    tmp_vcd = fsdb_make_temp(error_message);
    if (!tmp_vcd) {
        g_free(fsdb2vcd_path);
        g_free(vcd2fst_path);
        return 1;
    }

    /* Run fsdb2vcd: fsdb_path -> tmp_vcd */
    argv[0] = fsdb2vcd_path;
    argv[1] = (char *)fsdb_path;
    argv[2] = tmp_vcd;
    argv[3] = NULL;

    ok = fsdb_run_cmd(argv, error_message);
    if (!ok) {
        unlink(tmp_vcd);
        g_free(tmp_vcd);
        g_free(fsdb2vcd_path);
        g_free(vcd2fst_path);
        return 1;
    }

    /* Run vcd2fst: tmp_vcd -> fst_path */
    argv[0] = vcd2fst_path;
    argv[1] = "-v";
    argv[2] = tmp_vcd;
    argv[3] = "-f";
    argv[4] = (char *)fst_path;
    argv[5] = NULL;

    ok = fsdb_run_cmd(argv, error_message);
    unlink(tmp_vcd);
    g_free(tmp_vcd);
    g_free(fsdb2vcd_path);
    g_free(vcd2fst_path);

    return ok ? 0 : 1;
}

static void fsdb_external_free_error(char *error_message)
{
    g_free(error_message);
}

static const GtkwaveFsdbPluginInfo fsdb_external_info = {
    GTKWAVE_FSDB_PLUGIN_API_VERSION,
    "fsdb-external",
    "0.1",
    "gtkwave",
};

static const GtkwaveFsdbPluginApi fsdb_external_api = {
    GTKWAVE_FSDB_PLUGIN_API_VERSION,
    &fsdb_external_info,
    fsdb_external_convert_to_fst,
    fsdb_external_free_error,
};

const GtkwaveFsdbPluginApi *gtkwave_fsdb_plugin_get_api(void)
{
    return &fsdb_external_api;
}
