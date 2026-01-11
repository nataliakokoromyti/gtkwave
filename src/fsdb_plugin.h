#ifndef GTKWAVE_FSDB_PLUGIN_H
#define GTKWAVE_FSDB_PLUGIN_H

#include "fsdb_plugin_api.h"

char *fsdb_plugin_convert_to_fst(const char *fsdb_path,
                                 const char *plugin_path,
                                 char **error_message);
void fsdb_plugin_cleanup_temp_file(const char *path);

#endif
