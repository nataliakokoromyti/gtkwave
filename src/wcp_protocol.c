/*
 * WCP Protocol Implementation for GTKWave
 * 
 * Handles JSON parsing of incoming commands and generation of responses.
 */

#include "wcp_protocol.h"
#include <string.h>

/* Supported commands for greeting */
static const gchar *supported_commands[] = {
    "get_item_list",
    "get_item_info",
    "set_item_color",
    "add_variables",
    "add_scope",
    "add_items",
    "add_markers",
    "remove_items",
    "focus_item",
    "clear",
    "set_viewport_to",
    "set_viewport_range",
    "zoom_to_fit",
    "load",
    "reload",
    "shutdown",
    NULL
};

const gchar** wcp_get_supported_commands(void)
{
    return supported_commands;
}

/* ============================================================================
 * Command Parsing
 * ============================================================================ */

static WcpCommandType parse_command_type(const gchar *cmd_str)
{
    if (!cmd_str) return WCP_CMD_UNKNOWN;
    
    if (g_str_equal(cmd_str, "get_item_list"))      return WCP_CMD_GET_ITEM_LIST;
    if (g_str_equal(cmd_str, "get_item_info"))      return WCP_CMD_GET_ITEM_INFO;
    if (g_str_equal(cmd_str, "set_item_color"))     return WCP_CMD_SET_ITEM_COLOR;
    if (g_str_equal(cmd_str, "add_variables"))      return WCP_CMD_ADD_VARIABLES;
    if (g_str_equal(cmd_str, "add_scope"))          return WCP_CMD_ADD_SCOPE;
    if (g_str_equal(cmd_str, "add_items"))          return WCP_CMD_ADD_ITEMS;
    if (g_str_equal(cmd_str, "add_markers"))        return WCP_CMD_ADD_MARKERS;
    if (g_str_equal(cmd_str, "remove_items"))       return WCP_CMD_REMOVE_ITEMS;
    if (g_str_equal(cmd_str, "focus_item"))         return WCP_CMD_FOCUS_ITEM;
    if (g_str_equal(cmd_str, "clear"))              return WCP_CMD_CLEAR;
    if (g_str_equal(cmd_str, "set_viewport_to"))    return WCP_CMD_SET_VIEWPORT_TO;
    if (g_str_equal(cmd_str, "set_viewport_range")) return WCP_CMD_SET_VIEWPORT_RANGE;
    if (g_str_equal(cmd_str, "zoom_to_fit"))        return WCP_CMD_ZOOM_TO_FIT;
    if (g_str_equal(cmd_str, "load"))               return WCP_CMD_LOAD;
    if (g_str_equal(cmd_str, "reload"))             return WCP_CMD_RELOAD;
    if (g_str_equal(cmd_str, "shutdown"))           return WCP_CMD_SHUTDOWN;
    
    return WCP_CMD_UNKNOWN;
}

static GArray* parse_id_array(JsonArray *arr)
{
    GArray *ids = g_array_new(FALSE, FALSE, sizeof(WcpDisplayedItemRef));
    guint len = json_array_get_length(arr);
    
    for (guint i = 0; i < len; i++) {
        WcpDisplayedItemRef ref;
        ref.id = (guint)json_array_get_int_element(arr, i);
        g_array_append_val(ids, ref);
    }
    
    return ids;
}

static GPtrArray* parse_string_array(JsonArray *arr)
{
    GPtrArray *strings = g_ptr_array_new_with_free_func(g_free);
    guint len = json_array_get_length(arr);
    
    for (guint i = 0; i < len; i++) {
        const gchar *str = json_array_get_string_element(arr, i);
        g_ptr_array_add(strings, g_strdup(str));
    }
    
    return strings;
}

static GArray* parse_marker_array(JsonArray *arr)
{
    GArray *markers = g_array_new(FALSE, TRUE, sizeof(WcpMarkerInfo));
    guint len = json_array_get_length(arr);
    
    for (guint i = 0; i < len; i++) {
        JsonObject *marker_obj = json_array_get_object_element(arr, i);
        WcpMarkerInfo marker = {0};
        
        marker.time = json_object_get_int_member(marker_obj, "time");
        
        if (json_object_has_member(marker_obj, "name")) {
            marker.name = g_strdup(json_object_get_string_member(marker_obj, "name"));
        }
        
        if (json_object_has_member(marker_obj, "move_focus")) {
            marker.move_focus = json_object_get_boolean_member(marker_obj, "move_focus");
        }
        
        g_array_append_val(markers, marker);
    }
    
    return markers;
}

WcpCommand* wcp_parse_command(const gchar *json_str, GError **error)
{
    JsonParser *parser = json_parser_new();
    WcpCommand *cmd = NULL;
    
    if (!json_parser_load_from_data(parser, json_str, -1, error)) {
        g_object_unref(parser);
        return NULL;
    }
    
    JsonNode *root = json_parser_get_root(parser);
    if (!JSON_NODE_HOLDS_OBJECT(root)) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "WCP message must be a JSON object");
        g_object_unref(parser);
        return NULL;
    }
    
    JsonObject *obj = json_node_get_object(root);
    
    /* Check message type */
    const gchar *msg_type = json_object_get_string_member(obj, "type");
    if (!msg_type || !g_str_equal(msg_type, "command")) {
        /* Could be a greeting - handle separately */
        if (msg_type && g_str_equal(msg_type, "greeting")) {
            /* Client greeting - we just acknowledge it */
            cmd = g_new0(WcpCommand, 1);
            cmd->type = WCP_CMD_UNKNOWN;  /* Special case for greeting */
            g_object_unref(parser);
            return cmd;
        }
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Unknown message type: %s", msg_type ? msg_type : "(null)");
        g_object_unref(parser);
        return NULL;
    }
    
    /* Get command name */
    const gchar *cmd_name = json_object_get_string_member(obj, "command");
    WcpCommandType cmd_type = parse_command_type(cmd_name);
    
    if (cmd_type == WCP_CMD_UNKNOWN) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_INVALID_DATA,
                    "Unknown command: %s", cmd_name ? cmd_name : "(null)");
        g_object_unref(parser);
        return NULL;
    }
    
    cmd = g_new0(WcpCommand, 1);
    cmd->type = cmd_type;
    
    /* Parse command-specific fields */
    switch (cmd_type) {
        case WCP_CMD_GET_ITEM_INFO:
        case WCP_CMD_REMOVE_ITEMS:
            if (json_object_has_member(obj, "ids")) {
                JsonArray *arr = json_object_get_array_member(obj, "ids");
                cmd->data.item_refs.ids = parse_id_array(arr);
            }
            break;
            
        case WCP_CMD_SET_ITEM_COLOR:
            cmd->data.set_color.id.id = (guint)json_object_get_int_member(obj, "id");
            cmd->data.set_color.color = g_strdup(json_object_get_string_member(obj, "color"));
            break;
            
        case WCP_CMD_ADD_VARIABLES:
            if (json_object_has_member(obj, "variables")) {
                JsonArray *arr = json_object_get_array_member(obj, "variables");
                cmd->data.add_vars.variables = parse_string_array(arr);
            }
            break;
            
        case WCP_CMD_ADD_SCOPE:
            cmd->data.add_scope.scope = g_strdup(json_object_get_string_member(obj, "scope"));
            if (json_object_has_member(obj, "recursive")) {
                cmd->data.add_scope.recursive = json_object_get_boolean_member(obj, "recursive");
            }
            break;
            
        case WCP_CMD_ADD_ITEMS:
            if (json_object_has_member(obj, "items")) {
                JsonArray *arr = json_object_get_array_member(obj, "items");
                cmd->data.add_items.items = parse_string_array(arr);
            }
            if (json_object_has_member(obj, "recursive")) {
                cmd->data.add_items.recursive = json_object_get_boolean_member(obj, "recursive");
            }
            break;
            
        case WCP_CMD_ADD_MARKERS:
            if (json_object_has_member(obj, "markers")) {
                JsonArray *arr = json_object_get_array_member(obj, "markers");
                cmd->data.add_markers.markers = parse_marker_array(arr);
            }
            break;
            
        case WCP_CMD_SET_VIEWPORT_TO:
            cmd->data.viewport_to.timestamp = json_object_get_int_member(obj, "timestamp");
            break;
            
        case WCP_CMD_SET_VIEWPORT_RANGE:
            cmd->data.viewport_range.start = json_object_get_int_member(obj, "start");
            cmd->data.viewport_range.end = json_object_get_int_member(obj, "end");
            break;
            
        case WCP_CMD_FOCUS_ITEM:
            cmd->data.focus.id.id = (guint)json_object_get_int_member(obj, "id");
            break;
            
        case WCP_CMD_LOAD:
            cmd->data.load.source = g_strdup(json_object_get_string_member(obj, "source"));
            break;
            
        case WCP_CMD_ZOOM_TO_FIT:
            if (json_object_has_member(obj, "viewport_idx")) {
                cmd->data.zoom.viewport_idx = (guint)json_object_get_int_member(obj, "viewport_idx");
            }
            break;
            
        default:
            /* Commands with no additional data */
            break;
    }
    
    g_object_unref(parser);
    return cmd;
}

void wcp_command_free(WcpCommand *cmd)
{
    if (!cmd) return;
    
    switch (cmd->type) {
        case WCP_CMD_GET_ITEM_INFO:
        case WCP_CMD_REMOVE_ITEMS:
            if (cmd->data.item_refs.ids) {
                g_array_free(cmd->data.item_refs.ids, TRUE);
            }
            break;
            
        case WCP_CMD_SET_ITEM_COLOR:
            g_free(cmd->data.set_color.color);
            break;
            
        case WCP_CMD_ADD_VARIABLES:
            if (cmd->data.add_vars.variables) {
                g_ptr_array_free(cmd->data.add_vars.variables, TRUE);
            }
            break;
            
        case WCP_CMD_ADD_SCOPE:
            g_free(cmd->data.add_scope.scope);
            break;
            
        case WCP_CMD_ADD_ITEMS:
            if (cmd->data.add_items.items) {
                g_ptr_array_free(cmd->data.add_items.items, TRUE);
            }
            break;
            
        case WCP_CMD_ADD_MARKERS:
            if (cmd->data.add_markers.markers) {
                for (guint i = 0; i < cmd->data.add_markers.markers->len; i++) {
                    WcpMarkerInfo *m = &g_array_index(cmd->data.add_markers.markers, 
                                                       WcpMarkerInfo, i);
                    g_free(m->name);
                }
                g_array_free(cmd->data.add_markers.markers, TRUE);
            }
            break;
            
        case WCP_CMD_LOAD:
            g_free(cmd->data.load.source);
            break;
            
        default:
            break;
    }
    
    g_free(cmd);
}

/* ============================================================================
 * Response Generation
 * ============================================================================ */

gchar* wcp_create_greeting(void)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "greeting");
    
    json_builder_set_member_name(builder, "version");
    json_builder_add_string_value(builder, WCP_VERSION);
    
    json_builder_set_member_name(builder, "commands");
    json_builder_begin_array(builder);
    for (const gchar **cmd = supported_commands; *cmd != NULL; cmd++) {
        json_builder_add_string_value(builder, *cmd);
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, root);
    gchar *json_str = json_generator_to_data(gen, NULL);
    
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
    
    return json_str;
}

gchar* wcp_create_ack(void)
{
    return g_strdup("{\"type\":\"response\",\"command\":\"ack\"}");
}

gchar* wcp_create_error(const gchar *error_type, 
                        const gchar *message,
                        GPtrArray *arguments)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "error");
    
    json_builder_set_member_name(builder, "error");
    json_builder_add_string_value(builder, error_type);
    
    json_builder_set_member_name(builder, "message");
    json_builder_add_string_value(builder, message);
    
    json_builder_set_member_name(builder, "arguments");
    json_builder_begin_array(builder);
    if (arguments) {
        for (guint i = 0; i < arguments->len; i++) {
            json_builder_add_string_value(builder, g_ptr_array_index(arguments, i));
        }
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, root);
    gchar *json_str = json_generator_to_data(gen, NULL);
    
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
    
    return json_str;
}

gchar* wcp_create_item_list_response(GArray *ids)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "response");
    
    json_builder_set_member_name(builder, "command");
    json_builder_add_string_value(builder, "get_item_list");
    
    json_builder_set_member_name(builder, "ids");
    json_builder_begin_array(builder);
    if (ids) {
        for (guint i = 0; i < ids->len; i++) {
            WcpDisplayedItemRef *ref = &g_array_index(ids, WcpDisplayedItemRef, i);
            json_builder_add_int_value(builder, ref->id);
        }
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, root);
    gchar *json_str = json_generator_to_data(gen, NULL);
    
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
    
    return json_str;
}

gchar* wcp_create_item_info_response(GPtrArray *items)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "response");
    
    json_builder_set_member_name(builder, "command");
    json_builder_add_string_value(builder, "get_item_info");
    
    json_builder_set_member_name(builder, "results");
    json_builder_begin_array(builder);
    if (items) {
        for (guint i = 0; i < items->len; i++) {
            WcpItemInfo *info = g_ptr_array_index(items, i);
            json_builder_begin_object(builder);
            
            json_builder_set_member_name(builder, "name");
            json_builder_add_string_value(builder, info->name);
            
            json_builder_set_member_name(builder, "type");
            json_builder_add_string_value(builder, info->type);
            
            json_builder_set_member_name(builder, "id");
            json_builder_add_int_value(builder, info->id.id);
            
            json_builder_end_object(builder);
        }
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, root);
    gchar *json_str = json_generator_to_data(gen, NULL);
    
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
    
    return json_str;
}

gchar* wcp_create_add_items_response_for(const gchar *command, GArray *ids)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "response");
    
    json_builder_set_member_name(builder, "command");
    json_builder_add_string_value(builder, command ? command : "add_items");
    
    json_builder_set_member_name(builder, "ids");
    json_builder_begin_array(builder);
    if (ids) {
        for (guint i = 0; i < ids->len; i++) {
            WcpDisplayedItemRef *ref = &g_array_index(ids, WcpDisplayedItemRef, i);
            json_builder_add_int_value(builder, ref->id);
        }
    }
    json_builder_end_array(builder);
    
    json_builder_end_object(builder);
    
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, root);
    gchar *json_str = json_generator_to_data(gen, NULL);
    
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
    
    return json_str;
}

gchar* wcp_create_add_items_response(GArray *ids)
{
    return wcp_create_add_items_response_for("add_items", ids);
}

/* ============================================================================
 * Event Generation
 * ============================================================================ */

gchar* wcp_create_waveforms_loaded_event(const gchar *source)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "event");
    
    json_builder_set_member_name(builder, "event");
    json_builder_add_string_value(builder, "waveforms_loaded");
    
    json_builder_set_member_name(builder, "source");
    json_builder_add_string_value(builder, source);
    
    json_builder_end_object(builder);
    
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, root);
    gchar *json_str = json_generator_to_data(gen, NULL);
    
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
    
    return json_str;
}

gchar* wcp_create_goto_declaration_event(const gchar *variable)
{
    JsonBuilder *builder = json_builder_new();
    
    json_builder_begin_object(builder);
    json_builder_set_member_name(builder, "type");
    json_builder_add_string_value(builder, "event");
    
    json_builder_set_member_name(builder, "event");
    json_builder_add_string_value(builder, "goto_declaration");
    
    json_builder_set_member_name(builder, "variable");
    json_builder_add_string_value(builder, variable);
    
    json_builder_end_object(builder);
    
    JsonNode *root = json_builder_get_root(builder);
    JsonGenerator *gen = json_generator_new();
    json_generator_set_root(gen, root);
    gchar *json_str = json_generator_to_data(gen, NULL);
    
    json_node_free(root);
    g_object_unref(gen);
    g_object_unref(builder);
    
    return json_str;
}
