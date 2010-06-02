/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2009, Kalisko Project Leaders
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

#include <gtk/gtk.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "module.h"
#include "modules/gtk+/gtk+.h"
#include "modules/lang_lua/lang_lua.h"
#include "timer.h"
#include "api.h"

MODULE_NAME("lua_console");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical Lua console using GTK+");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 1, 2), MODULE_DEPENDENCY("lang_lua", 0, 5, 2));

// Columns
typedef enum {
		ROW_TIME,
		ROW_MESSAGE,
		ROW_MESSAGE_TYPE,
		N_ROWS
} PhpConsoleRow;

typedef enum {
		MESSAGE_IN,
		MESSAGE_OUT,
		MESSAGE_LOG
} PhpConsoleMessageType;

static void appendMessage(char *message, PhpConsoleMessageType isInType);
static gboolean closeWindow(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean inputActivate(GtkWidget *widget, GdkEvent *event, gpointer data);
static void formatMessageCell(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);

static GtkWidget *window;
static GtkWidget *list;
static GtkListStore *store;
static GtkWidget *vLayout;
static GtkWidget *scroll;
static GtkWidget *input;
static unsigned int lines;

MODULE_INIT
{
	lines = 0;

	// window
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Kalisko Lua console");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(closeWindow), NULL);

	// vertical layout
	vLayout = gtk_vbox_new(false, 1);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vLayout));

	// scroll
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vLayout), GTK_WIDGET(scroll));

	input = gtk_entry_new();
	GtkRcStyle *style = gtk_widget_get_modifier_style(GTK_WIDGET(input));
	PangoFontDescription *font = pango_font_description_from_string("Monospace Normal");
	style->font_desc = font;
	gtk_widget_modify_style(GTK_WIDGET(input), style);

	gtk_container_add(GTK_CONTAINER(vLayout), GTK_WIDGET(input));
	gtk_signal_connect(GTK_OBJECT(input), "activate", GTK_SIGNAL_FUNC(inputActivate), NULL);
	gtk_box_set_child_packing(GTK_BOX(vLayout), GTK_WIDGET(input), false, true, 0, GTK_PACK_END);

	// list
	list = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(list));

	// list columns
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), gtk_tree_view_column_new_with_attributes("Timestamp", gtk_cell_renderer_text_new(), "text", ROW_TIME, NULL));
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Message", renderer, "text", ROW_MESSAGE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	gtk_tree_view_column_set_cell_data_func(column, renderer, &formatMessageCell, NULL, NULL);

	// create store
	store = gtk_list_store_new(N_ROWS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));

	// show everything
	gtk_widget_show_all(GTK_WIDGET(window));

	// run
	$(void, gtk+, runGtkLoop)();

	return true;
}

MODULE_FINALIZE
{
	gtk_widget_destroy(GTK_WIDGET(window));
}

static void appendMessage(char *message, PhpConsoleMessageType type)
{
	GTimeVal *now = ALLOCATE_OBJECT(GTimeVal);
	g_get_current_time(now);
	char *dateTime = g_time_val_to_iso8601(now);

	GtkTreeIter iter;
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, ROW_TIME, dateTime, ROW_MESSAGE, message, ROW_MESSAGE_TYPE, type, -1);

	GtkTreePath	*path = gtk_tree_path_new_from_indices(lines++, -1);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(list), path, NULL, TRUE, 0.0, 0.0);
}

static gboolean inputActivate(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	char *command = (char *) gtk_entry_get_text(GTK_ENTRY(input));
	appendMessage(command, MESSAGE_IN);

	if(!$(bool, lang_lua, evaluateLua)(command)) {
		GString *err = g_string_new("Lua error: ");
		char *ret = $(char *, lang_lua, popLuaString)();
		g_string_append(err, ret);
		appendMessage(err->str, MESSAGE_LOG);
		free(ret);
		g_string_free(err, true);
	} else {
		char *ret = $(char *, lang_lua, popLuaString)();

		if(ret != NULL) {
			appendMessage(ret, MESSAGE_OUT);
			free(ret);
		}
	}

	gtk_entry_set_text(GTK_ENTRY(input), "");

	return true;
}

static gboolean closeWindow(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	$$(void, exitGracefully)();
	return true;
}

static void formatMessageCell(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data) // GtkTreeCellDataFunc
{
	PhpConsoleMessageType type;
	gtk_tree_model_get(tree_model, iter, ROW_MESSAGE_TYPE, &type, -1);

	switch(type) {
		case MESSAGE_IN:
			g_object_set(G_OBJECT(renderer), "foreground", "#0000ff", NULL);
			g_object_set(G_OBJECT(renderer), "weight", 800, NULL);
			g_object_set(G_OBJECT(renderer), "family", "Monospace", NULL);
		break;
		case MESSAGE_OUT:
			g_object_set(G_OBJECT(renderer), "foreground", "#000000", NULL);
			g_object_set(G_OBJECT(renderer), "weight", 400, NULL);
			g_object_set(G_OBJECT(renderer), "family", "Sans", NULL);
		break;
		case MESSAGE_LOG:
			g_object_set(G_OBJECT(renderer), "foreground", "#ff0000", NULL);
			g_object_set(G_OBJECT(renderer), "weight", 800, NULL);
			g_object_set(G_OBJECT(renderer), "family", "Sans", NULL);
		break;
	}
}
