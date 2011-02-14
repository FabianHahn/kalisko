/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     @li Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     @li Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <gtk/gtk.h>

#include "dll.h"
#include "modules/gtk+/gtk+.h"
#include "modules/gtk+/builder.h"
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "modules/irc/irc.h"
#include "modules/event/event.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/irc_channel/irc_channel.h"
#include "modules/property_table/property_table.h"
#include "api.h"

MODULE_NAME("irc_client");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical IRC client using GTK+");
MODULE_VERSION(0, 2, 7);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 2, 6), MODULE_DEPENDENCY("store", 0, 6, 10), MODULE_DEPENDENCY("config", 0, 3, 9), MODULE_DEPENDENCY("irc", 0, 4, 6), MODULE_DEPENDENCY("event", 0, 3, 0), MODULE_DEPENDENCY("irc_parser", 0, 1, 4), MODULE_DEPENDENCY("irc_channel", 0, 1, 8), MODULE_DEPENDENCY("property_table", 0, 0, 1));

typedef struct {
	/** The name of the IRC client connection */
	char *name;
	/** The text buffer for this connection */
	GtkTextBuffer *buffer;
	/** The IRC connection for this connection */
	IrcConnection *connection;
	/** A table of channels for this connection */
	GHashTable *channels;
} IrcClientConnection;

typedef struct {
	/** The name of the IRC client connection channel */
	char *name;
	/** The text buffer for this channel */
	GtkTextBuffer *buffer;
} IrcClientConnectionChannel;

typedef enum {
	SIDE_TREE_NAME_COLUMN,
	SIDE_TREE_TYPE_COLUMN,
	SIDE_TREE_ICON_COLUMN
} SideTreeColumns;

typedef enum {
	CHAT_MESSAGE_CONNECTION_LINE,
	CHAT_MESSAGE_CONNECTION_SEND
} ChatMessageType;

typedef enum {
	CHAT_ELEMENT_STATUS,
	CHAT_ELEMENT_CONNECTION,
	CHAT_ELEMENT_CHANNEL
} ChatElementType;

static void listener_channel(void *subject, const char *event, void *data, va_list args);
static void listener_ircSend(void *subject, const char *event, void *data, va_list args);
static void listener_ircLine(void *subject, const char *event, void *data, va_list args);
static void finalize();
static void addIrcClientConnection(char *name, Store *config);
static void refreshSideTree();
static void appendMessage(GtkTextBuffer *buffer, char *message, ChatMessageType type);
static void freeIrcClientConnection(void *connection_p);
static void freeIrcClientConnectionChannel(void *channel_p);
static int strpcmp(const void *p1, const void *p2);

/**
 * The window widget for the IRC client
 */
static GtkWidget *window;

/**
 * The chat output widget for the IRC client
 */
static GtkWidget *chat_output;

/**
 * The chat input widget for the IRC client
 */
static GtkWidget *chat_input;

/**
 * The side tree widget for the IRC client
 */
static GtkWidget *side_tree;

/**
 * The channel list widget for the IRC client
 */
static GtkWidget *channel_list;

/**
 * A hash table of connections for the IRC client
 */
static GHashTable *connections;

/**
 * The IRC client's config store
 */
static Store *client_config;

/**
 * The IRC client's status text buffer
 */
static GtkTextBuffer *status_buffer;

/** The IRC client's text tag table */
static GtkTextTagTable *tags;

/** The name of the active element */
static char *active_name = NULL;

/** The type of the active element */
static ChatElementType active_type = CHAT_ELEMENT_STATUS;

MODULE_INIT
{
	GString *path = g_string_new($$(char *, getExecutablePath)());
	g_string_append(path, "/modules/irc_client/irc_client.xml");

	GtkBuilder *builder = $(GtkBuilder *, gtk+, loadGtkBuilderGui)(path->str);

	g_string_free(path, true);

	if(builder == NULL) {
		LOG_ERROR("Failed to load IRC client GUI");
		return false;
	}

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	chat_output = GTK_WIDGET(gtk_builder_get_object(builder, "chat_output"));
	chat_input = GTK_WIDGET(gtk_builder_get_object(builder, "chat_input"));
	side_tree = GTK_WIDGET(gtk_builder_get_object(builder, "side_tree"));
	channel_list = GTK_WIDGET(gtk_builder_get_object(builder, "channel_list"));

	Store *config = $(Store *, config, getWritableConfig)();
	if((client_config = $(Store *, store, getStorePath)(config, "irc_client")) == NULL || client_config->type != STORE_ARRAY) {
		$(bool, store, deleteStorePath)(config, "irc_client");
		LOG_INFO("Writable config path 'irc_client' doesn't exist yet, creating...");
		client_config = $(Store *, store, createStore)();
		$(bool, store, setStorePath)(config, "irc_client", client_config);
	}

	// window
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

	// chat output
	GtkRcStyle *style = gtk_widget_get_modifier_style(chat_output);
	PangoFontDescription *font = pango_font_description_from_string("Monospace Normal");
	style->font_desc = font;
	gtk_widget_modify_style(chat_output, style);

	status_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_output));
	g_object_ref(status_buffer); // add reference to prevent freeing

	// tag table
	tags = gtk_text_buffer_get_tag_table(status_buffer);
	g_object_ref(tags);
	gtk_text_buffer_create_tag(status_buffer, "connection_send", "foreground", "blue", NULL);

	GString *welcome = $$(GString *, dumpVersion)(&_module_version);
	g_string_prepend(welcome, "Welcome to the Kalisko IRC client ");
	g_string_append(welcome, "!");
	gtk_text_buffer_set_text(status_buffer, welcome->str, -1);
	g_string_free(welcome, true);

	// chat input
	style = gtk_widget_get_modifier_style(chat_input);
	font = pango_font_description_from_string("Monospace Normal");
	style->font_desc = font;
	gtk_widget_modify_style(chat_input, style);

	// channel list
	style = gtk_widget_get_modifier_style(channel_list);
	font = pango_font_description_from_string("Monospace Normal");
	style->font_desc = font;
	gtk_widget_modify_style(channel_list, style);

	// side tree
	GtkCellRenderer *rendererPixbuf = gtk_cell_renderer_pixbuf_new();
	GtkCellRenderer *rendererText = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, rendererPixbuf, false);
	gtk_tree_view_column_add_attribute(column, rendererPixbuf, "stock-id", SIDE_TREE_ICON_COLUMN);
	gtk_tree_view_column_pack_start(column, rendererText, true);
	gtk_tree_view_column_add_attribute(column, rendererText, "text", SIDE_TREE_NAME_COLUMN);
	gtk_tree_view_append_column(GTK_TREE_VIEW(side_tree), column);
	g_object_set(G_OBJECT(side_tree), "headers_visible", false, NULL);

	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(side_tree));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_BROWSE);

	// show everything
	gtk_widget_show_all(GTK_WIDGET(window));

	// run
	$(void, gtk+, runGtkLoop)();

	connections = g_hash_table_new_full(&g_str_hash, &g_str_equal, NULL, &freeIrcClientConnection);

	// read connections from config
	Store *configConnections;
	if((configConnections = $(Store *, store, getStorePath)(client_config, "connections")) == NULL || configConnections->type != STORE_ARRAY) {
		$(bool, store, deleteStorePath)(client_config, "connections");
		LOG_INFO("Writable config path 'irc_client/connections' doesn't exist yet, creating...");
		configConnections = $(Store *, store, createStore)();
		$(bool, store, setStorePath)(client_config, "connections", configConnections);
	}

	GHashTableIter iter;
	g_hash_table_iter_init(&iter, configConnections->content.array);
	char *key;
	Store *value;
	while(g_hash_table_iter_next(&iter, (void **) &key, (void **) &value)) {
		addIrcClientConnection(key, value);
	}

	refreshSideTree();

	return true;
}

MODULE_FINALIZE
{
	finalize();
}

API gboolean irc_client_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	$$(void, exitGracefully)();
	return true;
}

void irc_client_side_tree_cursor_changed(GtkTreeView *tree_view, gpointer user_data)
{
	GtkTreeSelection *select = gtk_tree_view_get_selection(tree_view);
    GtkTreeIter iter;
    GtkTreeModel *model;

    if(gtk_tree_selection_get_selected(select, &model, &iter)) {
    	int type;
    	char *name;
    	gtk_tree_model_get(model, &iter, SIDE_TREE_TYPE_COLUMN, &type, SIDE_TREE_NAME_COLUMN, &name, -1);

    	if(type == 0) {
    		gtk_text_view_set_buffer(GTK_TEXT_VIEW(chat_output), status_buffer);
    		active_type = CHAT_ELEMENT_STATUS;
    		LOG_INFO("Switched to status");
    	} else if(type == 1) {
    		IrcClientConnection *connection = g_hash_table_lookup(connections, name);

    		if(connection != NULL) {
    			gtk_text_view_set_buffer(GTK_TEXT_VIEW(chat_output), connection->buffer);
    			active_type = CHAT_ELEMENT_CONNECTION;
    			active_name = connection->name;
    			LOG_INFO("Switched to connection '%s'", name);
    		} else {
    			LOG_ERROR("Failed to lookup IRC client connection '%s'", name);
    		}
    	} else if(type == 2) {
    		// retrieve parent connection
    		GtkTreeIter parent;
    		gtk_tree_model_iter_parent(model, &parent, &iter);

    		char *parentName;
    		gtk_tree_model_get(model, &parent, SIDE_TREE_NAME_COLUMN, &parentName, -1);

    		IrcClientConnection *connection = g_hash_table_lookup(connections, parentName);

    		if(connection != NULL) {
    			IrcClientConnectionChannel *channel = g_hash_table_lookup(connection->channels, name);

    			if(channel != NULL) {
    				gtk_text_view_set_buffer(GTK_TEXT_VIEW(chat_output), channel->buffer);
    				active_type = CHAT_ELEMENT_CHANNEL;
    				active_name = channel->name;
    				LOG_INFO("Switched to channel '%s' in connection '%s'", name, parentName);
    			} else {
    				LOG_ERROR("Failed to lookup channel '%s' in IRC client conneciction '%s'", name, parentName);
    			}
    		} else {
    			LOG_ERROR("Failed to lookup IRC client connection '%s'", parentName);
    		}
    	}
    }
}

API void irc_client_chat_output_scroll_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(chat_output));
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(chat_output), &end, 0.0, true, 1.0, 1.0);
}

API gboolean irc_client_chat_input_activate(GtkWidget *widget, gpointer data)
{
	char *command = (char *) gtk_entry_get_text(GTK_ENTRY(chat_input));

	switch(active_type) {
		case CHAT_ELEMENT_CONNECTION:
			do {
				IrcClientConnection *connection = g_hash_table_lookup(connections, active_name);
				$(bool, irc, ircSend)(connection->connection, "%s", command);
			} while(false);
		break;
		default:
			// nothing to do
		break;
	}

	gtk_entry_set_text(GTK_ENTRY(widget), "");

	return true;
}

static void listener_channel(void *subject, const char *event, void *data, va_list args)
{
	refreshSideTree();

	IrcClientConnection *connection = $(void *, property_table, getPropertyTableValue)(subject, "irc_client_connection");
	if(connection == NULL) {
		LOG_ERROR("Failed to look up IRC client connection for IRC connection");
		return;
	}

	if(g_strcmp0(event, "channel_join") == 0) { // join
		IrcChannel *channel = va_arg(args, IrcChannel *);
		IrcClientConnectionChannel *connectionChannel = ALLOCATE_OBJECT(IrcClientConnectionChannel);
		connectionChannel->name = strdup(channel->name);
		connectionChannel->buffer = gtk_text_buffer_new(tags);
		GString *text = g_string_new("");
		g_string_append_printf(text, "Created text buffer for channel '%s' in connection '%s'", connectionChannel->name, connection->name);
		gtk_text_buffer_set_text(connectionChannel->buffer, text->str, -1);
		g_string_free(text, true);

		// Add to channels table of connection
		g_hash_table_insert(connection->channels, connectionChannel->name, connectionChannel);
	} else { // part
		char *channel = va_arg(args, char *);
		g_hash_table_remove(connection->channels, channel);
	}
}

static void listener_ircSend(void *subject, const char *event, void *data, va_list args)
{
	char *message = va_arg(args, char *);

	IrcClientConnection *clientConnection = data;
	appendMessage(clientConnection->buffer, message, CHAT_MESSAGE_CONNECTION_SEND);
}

static void listener_ircLine(void *subject, const char *event, void *data, va_list args)
{
	IrcMessage *message = va_arg(args, IrcMessage *);

	IrcClientConnection *clientConnection = data;
	appendMessage(clientConnection->buffer, message->raw_message, CHAT_MESSAGE_CONNECTION_LINE);
}

static void finalize()
{
	gtk_widget_destroy(GTK_WIDGET(window));
	g_object_unref(status_buffer);
	g_object_unref(tags);
	g_hash_table_destroy(connections);
}

/**
 * Adds an IRC client connection to the IRC client
 *
 * @param name			the name of the connection to add
 * @param config		a configuration store for the IRC connection
 */
static void addIrcClientConnection(char *name, Store *config)
{
	IrcClientConnection *connection = ALLOCATE_OBJECT(IrcClientConnection);
	connection->name = strdup(name);
	connection->channels = g_hash_table_new_full(&g_str_hash, &g_str_equal, NULL, &freeIrcClientConnectionChannel);

	if((connection->connection = $(IrcConnection *, irc, createIrcConnectionByStore)(config)) == NULL) {
		LOG_ERROR("Failed to create IRC client connection '%s', aborting", name);
		free(connection->name);
		free(connection);
		return;
	}

	// Set reverse property
	$(bool, property_table, setPropertyTableValue)(connection->connection, "irc_client_connection", connection);

	// Enable channel tracking
	$(bool, irc_channel, enableChannelTracking)(connection->connection);

	// Attach to events
	$(void, event, attachEventListener)(connection->connection, "channel_join", connection, &listener_channel);
	$(void, event, attachEventListener)(connection->connection, "channel_part", connection, &listener_channel);
	$(void, event, attachEventListener)(connection->connection, "line", connection, &listener_ircLine);
	$(void, event, attachEventListener)(connection->connection, "send", connection, &listener_ircSend);

	// Create text buffer for the connection
	connection->buffer = gtk_text_buffer_new(tags);
	GString *text = g_string_new("");
	g_string_append_printf(text, "Created text buffer for connection '%s'", name);
	gtk_text_buffer_set_text(connection->buffer, text->str, -1);
	g_string_free(text, true);

	// Add to connections table
	g_hash_table_insert(connections, connection->name, connection);

	LOG_INFO("Added IRC client connection '%s'", name);
}

/**
 * Refeshes the side tree of the IRC client
 */
static void refreshSideTree()
{
	GtkTreeStore *treestore = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(side_tree)));
	gtk_tree_store_clear(treestore);

	// Add status entry
	GtkTreeIter child;
	gtk_tree_store_append(treestore, &child, NULL);
	gtk_tree_store_set(treestore, &child, SIDE_TREE_NAME_COLUMN, "Status", SIDE_TREE_TYPE_COLUMN, 0, SIDE_TREE_ICON_COLUMN, GTK_STOCK_INFO, -1);

	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(side_tree));
	gtk_tree_selection_select_iter(select, &child); // select status

	// Prepare connections
	GHashTableIter iter;
	char *key;
	IrcClientConnection *value;
	g_hash_table_iter_init(&iter, connections);

	GPtrArray *entries = g_ptr_array_new();
	while(g_hash_table_iter_next(&iter, (void **) &key, (void **) &value)) {
		g_ptr_array_add(entries, key);
	}

	// Sort connection list
	qsort(entries->pdata, entries->len, sizeof(char *), &strpcmp);

	// Add connections
	for(int i = 0; i < entries->len; i++) {
		key = entries->pdata[i];

		GtkTreeIter connectionIter;
		gtk_tree_store_append(treestore, &connectionIter, NULL);
		gtk_tree_store_set(treestore, &connectionIter, SIDE_TREE_NAME_COLUMN, key, SIDE_TREE_TYPE_COLUMN, 1, SIDE_TREE_ICON_COLUMN, GTK_STOCK_NETWORK, -1);

		IrcClientConnection *connection = g_hash_table_lookup(connections, key);
		GList *channels = $(GList *, irc_channel, getTrackedChannels)(connection->connection);

		for(GList *iter = channels; iter != NULL; iter = iter->next) {
			IrcChannel *channel = iter->data;
			GtkTreeIter channelIter;
			gtk_tree_store_append(treestore, &channelIter, &connectionIter);
			gtk_tree_store_set(treestore, &channelIter, SIDE_TREE_NAME_COLUMN, channel->name, SIDE_TREE_TYPE_COLUMN, 2, SIDE_TREE_ICON_COLUMN, GTK_STOCK_NO, -1);
		}

		g_list_free(channels);
	}

	g_ptr_array_free(entries, true);

	gtk_tree_view_expand_all(GTK_TREE_VIEW(side_tree));

	// Switch back to status
	gtk_text_view_set_buffer(GTK_TEXT_VIEW(chat_output), status_buffer);
	active_type = CHAT_ELEMENT_STATUS;
	LOG_INFO("Switched to status");
}

/**
 * Appends a message to a text buffer
 *
 * @param buffer		the buffer to append the message to
 * @param message		the message to append
 * @param type			the type of the message to insert
 */
static void appendMessage(GtkTextBuffer *buffer, char *message, ChatMessageType type)
{
	// deactivate text view as long as we're writing
	gtk_widget_set_sensitive(chat_output, false);

	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);

	GDateTime *now = g_date_time_new_now_local();
	GString *prefix = g_string_new("\n");
	g_string_append_printf(prefix, "[%02d:%02d:%02d] ", g_date_time_get_hour(now), g_date_time_get_minute(now), g_date_time_get_second(now));
	g_date_time_unref(now);
	gtk_text_buffer_insert(buffer, &end, prefix->str, -1);
	g_string_free(prefix, true);

	switch(type) {
		case CHAT_MESSAGE_CONNECTION_SEND:
			gtk_text_buffer_insert_with_tags_by_name(buffer, &end, message, -1, "connection_send", NULL);
		break;
		default:
			gtk_text_buffer_insert(buffer, &end, message, -1);
		break;
	}

	gtk_widget_set_sensitive(chat_output, true);
}

/**
 * A GDestroyNotify function to free an IRC client connection
 *
 * @param connection_p			a pointer to the IRC client connection to free
 */
static void freeIrcClientConnection(void *connection_p)
{
	IrcClientConnection *connection = connection_p;
	free(connection->name);
	$(void, property_table, freePropertyTable)(connection->connection);
	$(void, event, detachEventListener)(connection->connection, "channel_join", connection, &listener_channel);
	$(void, event, detachEventListener)(connection->connection, "channel_part", connection, &listener_channel);
	$(void, event, detachEventListener)(connection->connection, "line", connection, &listener_ircLine);
	$(void, event, detachEventListener)(connection->connection, "send", connection, &listener_ircSend);
	$(void, irc, freeIrcConnection)(connection->connection);
	g_object_unref(connection->buffer);
	g_hash_table_destroy(connection->channels);
	free(connection);
}

/**
 * A GDestroyNotify function to free an IRC client connection channel
 *
 * @param connection_p			a pointer to the IRC client connection channel to free
 */
static void freeIrcClientConnectionChannel(void *channel_p)
{
	IrcClientConnectionChannel *channel = channel_p;
	free(channel->name);
	g_object_unref(channel->buffer);
	free(channel);
}

static int strpcmp(const void *p1, const void *p2)
{
	return strcmp(* (char * const *) p1, * (char * const *) p2);
}
