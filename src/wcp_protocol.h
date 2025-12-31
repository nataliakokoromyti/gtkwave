/*
 * WCP (Waveform Control Protocol) for GTKWave
 * 
 * This implements the WCP protocol as defined by the Surfer project.
 * See: https://gitlab.com/surfer-project/surfer
 * 
 * WCP is a JSON-based protocol for controlling waveform viewers,
 * inspired by LSP (Language Server Protocol).
 */

#ifndef WCP_PROTOCOL_H
#define WCP_PROTOCOL_H

#include <glib.h>
#include <json-glib/json-glib.h>

/* WCP Protocol Version */
#define WCP_VERSION "1"

/* ============================================================================
 * WCP Message Types (Client -> Server)
 * ============================================================================ */

typedef enum {
    WCP_CMD_UNKNOWN = 0,
    
    /* Query commands */
    WCP_CMD_GET_ITEM_LIST,      /* Get list of displayed items */

    /* Modification commands */
    WCP_CMD_ADD_ITEMS,          /* Add variables or scopes */

    /* Navigation commands */
    WCP_CMD_SET_VIEWPORT_RANGE, /* Set view to time range */

    /* File commands */
    WCP_CMD_LOAD,               /* Load a waveform file */

    /* Control commands */
    WCP_CMD_SHUTDOWN,           /* Stop WCP server */
} WcpCommandType;

/* ============================================================================
 * Data Structures
 * ============================================================================ */

/* Reference to a displayed item (signal, marker, etc.) */
typedef struct {
    guint64 id;
} WcpDisplayedItemRef;

/* Parsed WCP command */
typedef struct {
    WcpCommandType type;
    
    /* Command-specific data */
    union {
        /* add_items */
        struct {
            GPtrArray *items;  /* Array of gchar* */
            gboolean recursive;
        } add_items;
        
        /* set_viewport_range */
        struct {
            gint64 start;
            gint64 end;
        } viewport_range;
        
        /* load */
        struct {
            gchar *source;
        } load;
    } data;
} WcpCommand;

/* ============================================================================
 * Function Prototypes
 * ============================================================================ */

/* Parse a JSON message into a WcpCommand structure */
WcpCommand* wcp_parse_command(const gchar *json_str, GError **error);

/* Free a WcpCommand structure */
void wcp_command_free(WcpCommand *cmd);

/* Create JSON response messages */
gchar* wcp_create_greeting(void);
gchar* wcp_create_ack(void);
gchar* wcp_create_error(const gchar *error_type, 
                        const gchar *message,
                        GPtrArray *arguments);
gchar* wcp_create_item_list_response(GArray *ids);
gchar* wcp_create_add_items_response_for(const gchar *command, GArray *ids);

#endif /* WCP_PROTOCOL_H */
