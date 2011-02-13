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
#include "api.h"

MODULE_NAME("irc_client");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical IRC client using GTK+");
MODULE_VERSION(0, 1, 3);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 2, 6), MODULE_DEPENDENCY("store", 0, 6, 10), MODULE_DEPENDENCY("config", 0, 3, 9));

static void finalize();

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
	side_tree = GTK_WIDGET(gtk_builder_get_object(builder, "site_tree"));
	channel_list = GTK_WIDGET(gtk_builder_get_object(builder, "channel_list"));

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

	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(side_tree));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

	// show everything
	gtk_widget_show_all(GTK_WIDGET(window));

	// run
	$(void, gtk+, runGtkLoop)();

	return true;
}

MODULE_FINALIZE
{
	finalize();
}

static void finalize()
{
	gtk_widget_destroy(GTK_WIDGET(window));
}
