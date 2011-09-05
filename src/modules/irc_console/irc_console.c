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
#include "log.h"
#include "module.h"
#include "modules/gtk+/gtk+.h"
#include "modules/irc/irc.h"
#include "modules/config/config.h"
#include "modules/store/path.h"
#include "modules/irc_parser/irc_parser.h"
#include "modules/event/event.h"
#include "timer.h"
#define API

MODULE_NAME("irc_console");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical IRC console using GTK+");
MODULE_VERSION(0, 1, 8);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("store", 0, 6, 0), MODULE_DEPENDENCY("config", 0, 3, 9), MODULE_DEPENDENCY("irc", 0, 2, 1), MODULE_DEPENDENCY("irc_parser", 0, 1, 0), MODULE_DEPENDENCY("gtk+", 0, 1, 2), MODULE_DEPENDENCY("event", 0, 1, 2));

// Columns
typedef enum {
		ROW_TIME,
		ROW_MESSAGE,
		ROW_MESSAGE_TYPE,
		N_ROWS
} IrcConsoleRow;

typedef enum {
		MESSAGE_SEND,
		MESSAGE_LINE
} IrcConsoleMessageType;

typedef struct {
	unsigned int lines;
	GtkWidget *list;
	GtkListStore *store;
} IrcConsoleTab;

static void createTab(char *name);
static void appendMessage(char *tabname, char *message, IrcConsoleMessageType isInType);
static gboolean closeWindow(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean inputActivate(GtkWidget *widget, gpointer data);
static void formatMessageCell(GtkTreeViewColumn *tree_column, GtkCellRenderer *cell, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data);
static void freeConsoleTab(void *tab_p);

static void listener_ircSend(void *subject, const char *event, void *data, va_list args);
static void listener_ircLine(void *subject, const char *event, void *data, va_list args);

static IrcConnection *irc;
static GHashTable *tabs;
static GtkWidget *window;
static GtkWidget *notebook;

MODULE_INIT
{
	Store *config = $(Store *, config, getConfigPath)("irc");

	if(config == NULL) {
		return false;
	}

	if((irc = $(IrcConnection *, irc, createIrcConnectionByStore)(config)) == NULL) {
		return false;
	}

	$(void, event, attachEventListener)(irc, "line", NULL, &listener_ircLine);
	$(void, event, attachEventListener)(irc, "send", NULL, &listener_ircSend);

	Store *param = $(Store *, store, getStorePath)(config, "throttle");

	if(param != NULL && param->type == STORE_INTEGER && param->content.integer > 0) { // throttle irc connection
		$(bool, irc, enableIrcConnectionThrottle)(irc);
	}

	tabs = g_hash_table_new_full(&g_str_hash, &g_str_equal, &free, &freeConsoleTab);

	// window
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "Kalisko IRC console");
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(closeWindow), NULL);

	notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), true);
	createTab("*status");

	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(notebook));

	// show everything
	gtk_widget_show_all(GTK_WIDGET(window));

	// run
	$(void, gtk+, runGtkLoop)();

	return true;
}

MODULE_FINALIZE
{
	$(void, event, detachEventListener)(irc, "line", NULL, &listener_ircLine);
	$(void, event, detachEventListener)(irc, "send", NULL, &listener_ircSend);

	$(void, irc, freeIrcConnection)(irc);

	gtk_widget_destroy(GTK_WIDGET(window));

	g_hash_table_destroy(tabs);
}

static void listener_ircSend(void *subject, const char *event, void *data, va_list args)
{
	IrcConnection *connection = subject;
	char *message = va_arg(args, char *);

	if(irc == connection) {
		appendMessage("*status", message, MESSAGE_SEND);
	}
}

static void listener_ircLine(void *subject, const char *event, void *data, va_list args)
{
	IrcConnection *connection = subject;
	IrcMessage *message = va_arg(args, IrcMessage *);

	if(irc != connection) {
		return;
	}

	appendMessage("*status", message->raw_message, MESSAGE_LINE);

	if(g_strcmp0(message->command, "JOIN") == 0) {
		IrcUserMask *mask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(message->prefix);
		if(mask != NULL) {
			char *nick = irc->nick;

			if(g_strcmp0(mask->nick, nick) == 0) { // it's-a-me!
				if(message->params != NULL && message->params[0] != NULL) { // usually, the channel is being sent straight as parameter
					createTab(message->params[0]);
				} else if(message->trailing != NULL) { // some other servers though, like znc, send it as trailing parameter...
					createTab(message->trailing);
				} else { // anything else can't possibly happen
					LOG_ERROR("No channel given in JOIN command");
				}
			}

			$(void, irc_parser, freeIrcUserMask)(mask);
		}
	} else if(g_strcmp0(message->command, "PRIVMSG") == 0) {
		if(message->params[0] != NULL) {
			IrcUserMask *mask = $(IrcUserMask *, irc_parser, parseIrcUserMask)(message->prefix);
			if(mask != NULL) {
				GString *msg = g_string_new("");
				g_string_append_printf(msg, "<%s> %s", mask->nick, message->trailing);
				appendMessage(message->params[0], msg->str, MESSAGE_LINE);
				g_string_free(msg, true);
				$(void, irc_parser, freeIrcUserMask)(mask);
			}
		}
	}
}

static void createTab(char *name)
{
	LOG_DEBUG("Creating IRC console tab '%s'", name);

	char *dupname = strdup(name);

	// vertical layout
	GtkWidget *vLayout = gtk_vbox_new(false, 1);

	// scroll
	GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vLayout), GTK_WIDGET(scroll));

	GtkWidget *input = gtk_entry_new();
	GtkRcStyle *style = gtk_widget_get_modifier_style(GTK_WIDGET(input));
	PangoFontDescription *font = pango_font_description_from_string("Monospace Normal");
	style->font_desc = font;
	gtk_widget_modify_style(GTK_WIDGET(input), style);

	gtk_container_add(GTK_CONTAINER(vLayout), GTK_WIDGET(input));
	g_signal_connect(GTK_OBJECT(input), "activate", GTK_SIGNAL_FUNC(inputActivate), dupname);
	gtk_box_set_child_packing(GTK_BOX(vLayout), GTK_WIDGET(input), false, true, 0, GTK_PACK_END);

	// list
	GtkWidget *list = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(list));

	// list columns
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), gtk_tree_view_column_new_with_attributes("Timestamp", gtk_cell_renderer_text_new(), "text", ROW_TIME, NULL));
	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Message", renderer, "text", ROW_MESSAGE, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(list), column);
	gtk_tree_view_column_set_cell_data_func(column, renderer, &formatMessageCell, NULL, NULL);

	// create store
	GtkListStore *store = gtk_list_store_new(N_ROWS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));

	GtkWidget *title = gtk_label_new(name);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), GTK_WIDGET(vLayout), GTK_WIDGET(title));

	IrcConsoleTab *tab = ALLOCATE_OBJECT(IrcConsoleTab);
	g_object_ref(G_OBJECT(list));
	tab->lines = 0;
	tab->list = list;
	tab->store = store;

	g_hash_table_insert(tabs, dupname, tab);

	gtk_widget_show_all(GTK_WIDGET(notebook));
}

static void appendMessage(char *tabname, char *message, IrcConsoleMessageType type)
{
	IrcConsoleTab *tab = g_hash_table_lookup(tabs, tabname);

	if(tab == NULL) {
		LOG_ERROR("Requested unknown tab '%s'", tabname);
		return;
	}

	GTimeVal *now = ALLOCATE_OBJECT(GTimeVal);
	g_get_current_time(now);
	char *dateTime = g_time_val_to_iso8601(now);

	GtkTreeIter iter;
	gtk_list_store_append(tab->store, &iter);
	gtk_list_store_set(tab->store, &iter, ROW_TIME, dateTime, ROW_MESSAGE, message, ROW_MESSAGE_TYPE, type, -1);

	GtkTreePath	*path = gtk_tree_path_new_from_indices(tab->lines++, -1);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(tab->list), path, NULL, TRUE, 0.0, 0.0);
}

static gboolean inputActivate(GtkWidget *widget, gpointer data)
{
	char *tab = (char *) data;
	char *command = (char *) gtk_entry_get_text(GTK_ENTRY(widget));

	if(g_strcmp0(tab, "*status") == 0) {
		$(void, irc, ircSend)(irc, "%s", command);
	} else {
		$(void, irc, ircSend)(irc, "PRIVMSG %s :%s", tab, command);
		char *nick = irc->nick;
		GString *msg = g_string_new("");
		g_string_append_printf(msg, "<%s> %s", nick, command);
		appendMessage(tab, msg->str, MESSAGE_SEND);
		g_string_free(msg, true);
	}


	gtk_entry_set_text(GTK_ENTRY(widget), "");

	return true;
}

static gboolean closeWindow(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	$$(void, exitGracefully)();
	return true;
}

static void formatMessageCell(GtkTreeViewColumn *tree_column, GtkCellRenderer *renderer, GtkTreeModel *tree_model, GtkTreeIter *iter, gpointer data) // GtkTreeCellDataFunc
{
	IrcConsoleMessageType type;
	gtk_tree_model_get(tree_model, iter, ROW_MESSAGE_TYPE, &type, -1);

	switch(type) {
		case MESSAGE_SEND:
			g_object_set(G_OBJECT(renderer), "foreground", "#0000ff", NULL);
			g_object_set(G_OBJECT(renderer), "weight", 400, NULL);
			g_object_set(G_OBJECT(renderer), "family", "Monospace", NULL);
		break;
		case MESSAGE_LINE:
			g_object_set(G_OBJECT(renderer), "foreground", "#000000", NULL);
			g_object_set(G_OBJECT(renderer), "weight", 400, NULL);
			g_object_set(G_OBJECT(renderer), "family", "Monospace", NULL);
		break;
	}
}

static void freeConsoleTab(void *tab_p)
{
	IrcConsoleTab *tab = (IrcConsoleTab *) tab_p;
	g_object_unref(G_OBJECT(tab->list));
	g_object_unref(G_OBJECT(tab->store));
	free(tab);
}
