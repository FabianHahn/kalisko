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
#include "timer.h"
#include "module.h"
#include "modules/gtk+/gtk+.h"

#include "api.h"

MODULE_NAME("GTK+");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Basic module for GTK+ bases Kalisko modules.");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 1, 0));

static gboolean closeWindow(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean doLog(GtkWidget *widget, GdkEvent *event, gpointer data);

HOOK_LISTENER(log);

GtkWidget *window;
GtkWidget *list;
GtkListStore *store;
GtkTreeIter *iter;
GtkWidget *vLayout;
GtkWidget *toolbar;
GtkWidget *scroll;

// Columns
enum {
		COLUMN_LOG_TYPE = 0,
		COLUMN_DATE_TIME,
		COLUMN_MESSAGE,
		COLUMN_COUNT
};

MODULE_INIT
{
	gtk_init(NULL, NULL);
	iter = ALLOCATE_OBJECT(GtkTreeIter);

	// window
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size(GTK_WIDGET(window), 850, 250);
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(closeWindow), NULL);

	// vertical layout
	vLayout = gtk_vbox_new(false, 1);
	gtk_container_add(GTK_CONTAINER(window), GTK_WIDGET(vLayout));

	// toolbar
	toolbar = gtk_toolbar_new();
	gtk_container_add(GTK_CONTAINER(vLayout), GTK_WIDGET(toolbar));
	gtk_container_child_set(GTK_CONTAINER(vLayout), GTK_WIDGET(toolbar), "expand", false, NULL);

	GtkToolItem *toolButton = gtk_tool_button_new(NULL, "Log it!");
	g_signal_connect(G_OBJECT(toolButton), "clicked", G_CALLBACK(doLog), NULL);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolButton), 0);

	// scroll
	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(vLayout), GTK_WIDGET(scroll));

	// list
	list = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(list));

	// list columns
	gtk_tree_view_append_column(GTK_TREE_VIEW(list),
		gtk_tree_view_column_new_with_attributes("Level", gtk_cell_renderer_pixbuf_new(), "pixbuf", COLUMN_LOG_TYPE, NULL)
	);

	gtk_tree_view_append_column(GTK_TREE_VIEW(list),
			gtk_tree_view_column_new_with_attributes("Timestamp", gtk_cell_renderer_text_new(), "text", COLUMN_DATE_TIME, NULL)
	);

	gtk_tree_view_append_column(GTK_TREE_VIEW(list),
			gtk_tree_view_column_new_with_attributes("Message", gtk_cell_renderer_text_new(), "text", COLUMN_MESSAGE, NULL)
	);

	// create store
	store = gtk_list_store_new(COLUMN_COUNT, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), GTK_TREE_MODEL(store));

	// show everything
	gtk_widget_show_all(GTK_WIDGET(window));

	HOOK_ATTACH(log, log);

	// run
	$(void, gtk+, runGtkLoop)();

	return true;
}

MODULE_FINALIZE
{
	HOOK_DETACH(log, log);
	gtk_widget_destroy(GTK_WIDGET(window));
	free(iter);
}

HOOK_LISTENER(log)
{
	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);

	GTimeVal *now = ALLOCATE_OBJECT(GTimeVal);
	g_get_current_time(now);
	char *dateTime = g_time_val_to_iso8601(now);

	GdkPixbuf *icon;
	switch(type) {
		case LOG_TYPE_ERROR:
			icon = gtk_widget_render_icon(GTK_WIDGET(window), GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU, NULL);
			break;
		case LOG_TYPE_WARNING:
			icon = gtk_widget_render_icon(GTK_WIDGET(window), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU, NULL);
			break;
		case LOG_TYPE_INFO:
			icon = gtk_widget_render_icon(GTK_WIDGET(window), GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU, NULL);
			break;
		case LOG_TYPE_DEBUG:
			icon = gtk_widget_render_icon(GTK_WIDGET(window), GTK_STOCK_INFO, GTK_ICON_SIZE_MENU, NULL);
			break;
	}

	gtk_list_store_append(store, iter);
	gtk_list_store_set(store, iter, COLUMN_LOG_TYPE, icon, COLUMN_DATE_TIME, dateTime, COLUMN_MESSAGE, message, -1);
}

static gboolean doLog(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	LOG_ERROR("Let's log!");
	return true;
}

static gboolean closeWindow(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	$$(void, exitGracefully)();
	return true;
}
