#include "../../src/fsdb_plugin_api.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <process.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

static int fsdb_run_cmd(const char *cmd, char **error_message)
{
    int rc = system(cmd);
    if (rc != 0 && error_message) {
        size_t len = snprintf(NULL, 0, "Command failed: %s", cmd);
        char *buf = (char *)malloc(len + 1);
        if (buf) {
            snprintf(buf, len + 1, "Command failed: %s", cmd);
            *error_message = buf;
        }
    }
    return rc == 0;
}

static char *fsdb_make_temp(const char *suffix)
{
    char tmpl[] = "gtkwave-fsdb-XXXXXX";
    char *buf = NULL;
    size_t len = strlen(tmpl) + 1;

    buf = (char *)malloc(len);
    if (!buf) {
        return NULL;
    }
    (void)suffix;
    snprintf(buf, len, "%s", tmpl);

#ifdef _WIN32
    {
        char tmpbuf[L_tmpnam];
        if (!tmpnam(tmpbuf)) {
            free(buf);
            return NULL;
        }
        snprintf(buf, len, "%s", tmpbuf);
    }
#else
    {
        int fd = mkstemp(buf);
        if (fd < 0) {
            free(buf);
            return NULL;
        }
        close(fd);
    }
#endif

    return buf;
}

static int fsdb_external_convert_to_fst(const char *fsdb_path,
                                        const char *fst_path,
                                        char **error_message)
{
    const char *fsdb2vcd = getenv("GTKWAVE_FSDB2VCD");
    const char *vcd2fst = getenv("GTKWAVE_VCD2FST");
    char *tmp_vcd = NULL;
    char *cmd = NULL;
    int ok = 0;

    if (!fsdb2vcd || !*fsdb2vcd) {
        fsdb2vcd = "fsdb2vcd_fast";
    }
    if (!vcd2fst || !*vcd2fst) {
        vcd2fst = "vcd2fst";
    }

    tmp_vcd = fsdb_make_temp(".vcd");
    if (!tmp_vcd) {
        if (error_message) {
            size_t len = snprintf(NULL,
                                  0,
                                  "Failed to create temp file: %s",
                                  strerror(errno));
            char *buf = (char *)malloc(len + 1);
            if (buf) {
                snprintf(buf, len + 1, "Failed to create temp file: %s", strerror(errno));
                *error_message = buf;
            }
        }
        return 1;
    }

    {
        size_t len = strlen(fsdb2vcd) + strlen(fsdb_path) + strlen(tmp_vcd) + 10;
        cmd = (char *)malloc(len);
        if (!cmd) {
            free(tmp_vcd);
            return 1;
        }
        snprintf(cmd, len, "%s \"%s\" \"%s\"", fsdb2vcd, fsdb_path, tmp_vcd);
    }

    ok = fsdb_run_cmd(cmd, error_message);
    free(cmd);
    if (!ok) {
        unlink(tmp_vcd);
        free(tmp_vcd);
        return 1;
    }

    {
        size_t len = strlen(vcd2fst) + strlen(tmp_vcd) + strlen(fst_path) + 12;
        cmd = (char *)malloc(len);
        if (!cmd) {
            unlink(tmp_vcd);
            free(tmp_vcd);
            return 1;
        }
        snprintf(cmd, len, "%s -v \"%s\" -f \"%s\"", vcd2fst, tmp_vcd, fst_path);
    }

    ok = fsdb_run_cmd(cmd, error_message);
    free(cmd);
    unlink(tmp_vcd);
    free(tmp_vcd);

    return ok ? 0 : 1;
}

static void fsdb_external_free_error(char *error_message)
{
    free(error_message);
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
