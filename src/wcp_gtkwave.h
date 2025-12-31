/*
 * GTKWave WCP Integration
 * 
 * This module bridges WCP commands to GTKWave's internal functions.
 * It translates WCP protocol messages into GTKWave API calls.
 * 
 * NOTE: This file requires access to GTKWave internals (globals.h, etc.)
 * and should be placed in src/ of the GTKWave source tree.
 */

#ifndef WCP_GTKWAVE_H
#define WCP_GTKWAVE_H

#include "wcp_server.h"

/* Global WCP server instance */
extern WcpServer *g_wcp_server;

/* ============================================================================
 * Initialization
 * ============================================================================ */

/**
 * Initialize WCP support for GTKWave
 * Call this from main() after GTKWave is initialized
 * 
 * @param port Port to listen on (0 for default 8765)
 * @return TRUE on success
 */
gboolean wcp_gtkwave_init(guint16 port);

/**
 * Initialize WCP in "initiate" mode - connect to existing client
 * 
 * @param host Host to connect to
 * @param port Port to connect to
 * @return TRUE on success
 */
gboolean wcp_gtkwave_initiate(const gchar *host, guint16 port);

/**
 * Shutdown WCP support
 * Call this before GTKWave exits
 */
void wcp_gtkwave_shutdown(void);

/* ============================================================================
 * Event Emission (call these from GTKWave code)
 * ============================================================================ */

/**
 * Notify WCP client that waveforms have been loaded
 * Call this after successful file load
 */
void wcp_gtkwave_notify_waveforms_loaded(const gchar *filename);

/**
 * Notify WCP client of goto-declaration request
 * Call this when user requests to jump to source
 */
void wcp_gtkwave_notify_goto_declaration(const gchar *variable);

#endif /* WCP_GTKWAVE_H */
