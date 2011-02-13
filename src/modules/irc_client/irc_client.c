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
#include "api.h"

MODULE_NAME("irc_client");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical IRC client using GTK+");
MODULE_VERSION(0, 1, 6);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 2, 6), MODULE_DEPENDENCY("store", 0, 6, 10), MODULE_DEPENDENCY("config", 0, 3, 9), MODULE_DEPENDENCY("irc", 0, 4, 6));

static void finalize();
static void addIrcClientConnection(char *name, Store *config);
static void refreshSideTree();
static void freeIrcClientConnection(void *connection_p);
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

typedef struct {
	/** The name of the IRC client connection */
	char *name;
	/** The IRC connection for this connection */
	IrcConnection *connection;
} IrcClientConnection;

typedef enum {
	SIDE_TREE_NAME_COLUMN,
	SIDE_TREE_TYPE_COLUMN,
	SIDE_TREE_ICON_COLUMN
} SideTreeColumns;

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
	if((client_config = $(Store *, store, getStorePath)(config, "irc_client")) == NULL) {
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
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

	// show everything
	gtk_widget_show_all(GTK_WIDGET(window));

	// run
	$(void, gtk+, runGtkLoop)();

	connections = g_hash_table_new_full(&g_str_hash, &g_str_equal, NULL, &freeIrcClientConnection);
	refreshSideTree();

	return true;
}

MODULE_FINALIZE
{
	finalize();
}

static void finalize()
{
	gtk_widget_destroy(GTK_WIDGET(window));
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

	if((connection->connection = $(IrcConnection *, irc, createIrcConnectionByStore)(config)) == NULL) {
		LOG_ERROR("Failed to create IRC client connection '%s', aborting", name);
		free(connection->name);
		free(connection);
		return;
	}

	// Add to connections table
	g_hash_table_insert(connections, connection->name, connections);

	refreshSideTree();
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

		GtkTreeIter child;
		gtk_tree_store_append(treestore, &child, NULL);
		gtk_tree_store_set(treestore, &child, SIDE_TREE_NAME_COLUMN, key, SIDE_TREE_TYPE_COLUMN, 1, SIDE_TREE_ICON_COLUMN, GTK_STOCK_NETWORK, -1);
	}

	g_ptr_array_free(entries, true);
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
	$(void, irc, freeIrcConnection)(connection->connection);
	free(connection);
}

static int strpcmp(const void *p1, const void *p2)
{
	return strcmp(* (char * const *) p1, * (char * const *) p2);
}
