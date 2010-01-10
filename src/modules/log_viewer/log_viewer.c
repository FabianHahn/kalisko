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
#include <assert.h>

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "timer.h"
#include "module.h"
#include "memory_alloc.h"
#include "modules/gtk+/gtk+.h"
#include "modules/config_standard/config_standard.h"
#include "modules/config_standard/util.h"

#include "api.h"
#include "modules/log_viewer/log_viewer.h"

MODULE_NAME("log_viewer");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Provides a widget and window to show log messages.");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 1, 2));

#define PERFORM_CONFIG_PATH "kalisko/loadModules"

HOOK_LISTENER(newLogMessage);

static gboolean closeWindow(GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean addLogMessage(GtkWidget *widget, GdkEvent *event, gpointer data);
static int cmpStringItems(const void *a, const void *b);

MODULE_INIT
{
	ConfigNodeValue *perform = $(ConfigNodeValue *, config_standard, getStandardConfigPathValue)(PERFORM_CONFIG_PATH);

	if(perform != NULL && perform->type == CONFIG_LIST &&
		g_queue_find_custom(perform->content.list, "log_viewer", (GCompareFunc)cmpStringItems) != NULL) {

		LogViewerWindow *window = newLogViewerWindow();
		g_signal_connect(G_OBJECT(window->window), "delete_event", G_CALLBACK(closeWindow), window);

		HOOK_ATTACH_EX(log, newLogMessage, window);

		// show and start
		gtk_widget_show_all(GTK_WIDGET(window->window));
		$(void, gtk+, runGtkLoop)();
	}

	return true;
}

MODULE_FINALIZE
{

}

/**
 * Creates a new log viewer struct.
 *
 * @return A new log viewer. Must be freed with freeGtkLogViewer().
 */
API LogViewer *newLogViewer()
{
	LogViewer *logViewer = ALLOCATE_OBJECT(LogViewer);
	logViewer->lines = 0;
	logViewer->listIter = ALLOCATE_OBJECT(GtkTreeIter);

	// vertical box
	logViewer->container = gtk_vbox_new(false, 0);

	// toolbar
	logViewer->toolbar = gtk_toolbar_new();
	gtk_container_add(GTK_CONTAINER(logViewer->container), GTK_WIDGET(logViewer->toolbar));
	gtk_container_child_set(GTK_CONTAINER(logViewer->container), GTK_WIDGET(logViewer->toolbar), "expand", false, NULL);

	GtkWidget *addLogMessageBtn = gtk_toolbar_append_item(GTK_TOOLBAR(logViewer->toolbar), "Log!", "Adds a new Log message", NULL, NULL, NULL, NULL);
	g_signal_connect(G_OBJECT(addLogMessageBtn), "clicked", G_CALLBACK(addLogMessage), NULL);

	// scroll window for list view
	logViewer->treeViewScrollbar = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(logViewer->treeViewScrollbar), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(logViewer->container), GTK_WIDGET(logViewer->treeViewScrollbar));

	// list view
	logViewer->treeView = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(logViewer->treeViewScrollbar), GTK_WIDGET(logViewer->treeView));

	// list view columns
	gtk_tree_view_append_column(GTK_TREE_VIEW(logViewer->treeView),
		gtk_tree_view_column_new_with_attributes("Level", gtk_cell_renderer_pixbuf_new(), "pixbuf", COLUMN_LOG_TYPE, NULL)
	);

	gtk_tree_view_append_column(GTK_TREE_VIEW(logViewer->treeView),
		gtk_tree_view_column_new_with_attributes("Timestamp", gtk_cell_renderer_text_new(), "text", COLUMN_DATE_TIME, NULL)
	);

	gtk_tree_view_append_column(GTK_TREE_VIEW(logViewer->treeView),
		gtk_tree_view_column_new_with_attributes("Message", gtk_cell_renderer_text_new(), "text", COLUMN_MESSAGE, NULL)
	);

	// list store
	logViewer->listStore = gtk_list_store_new(COLUMN_COUNT, GDK_TYPE_PIXBUF, GTK_TYPE_STRING, GTK_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(logViewer->treeView), GTK_TREE_MODEL(logViewer->listStore));

	return logViewer;
}

/**
 * Destorys and frees a log viewer.
 *
 * @param viewer	The log viewer to destroy
 */
API void freeLogViewer(LogViewer *viewer)
{
	g_object_unref(viewer->listStore);
	gtk_widget_destroy(viewer->container);
	free(viewer->listIter);
	free(viewer);
}

/**
 * Creates a new window with a log viewer instance.
 *
 * @return The newly created log viewer window. Must be freed with freeGtkLogViewerWindow().
 */
API LogViewerWindow *newLogViewerWindow()
{
	LogViewerWindow *window = ALLOCATE_OBJECT(LogViewerWindow);

	// window for log viewer
	window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window->window), "Kalisko Log Viewer");
	gtk_window_set_icon_name(GTK_WINDOW(window->window), GTK_STOCK_SELECT_ALL);
	gtk_window_set_default_size(GTK_WINDOW(window->window), 850, 250);

	// log viewer
	window->logViewer = newLogViewer();
	gtk_container_add(GTK_CONTAINER(window->window), GTK_WIDGET(window->logViewer->container));

	return window;
}

/**
 * Destroys and frees a log viewer window.
 *
 * @param window		The log viewer window to destroy.
 */
API void freeLogViewerWindow(LogViewerWindow *window)
{
	freeLogViewer(window->logViewer);
	gtk_widget_destroy(window->window);

	free(window);
}

API void logViewerAddMessage(LogViewer *logViewer, char *time, char *message, GdkPixbuf *icon)
{
	gtk_list_store_append(logViewer->listStore, logViewer->listIter);
	gtk_list_store_set(logViewer->listStore, logViewer->listIter, COLUMN_LOG_TYPE, icon, COLUMN_DATE_TIME, time, COLUMN_MESSAGE, message, -1);

	GtkTreePath *path = gtk_tree_path_new_from_indices(logViewer->lines++, -1);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(logViewer->treeView), path, 0, true, 0.0, 0.0);

	gtk_tree_path_free(path);
}

HOOK_LISTENER(newLogMessage)
{
	LogViewerWindow *window = (LogViewerWindow *)custom_data;

	LogType type = HOOK_ARG(LogType);
	char *message = HOOK_ARG(char *);

	GTimeVal *now = ALLOCATE_OBJECT(GTimeVal);
	g_get_current_time(now);
	char *dateTime = g_time_val_to_iso8601(now);

	GdkPixbuf *icon;
	switch(type) {
		case LOG_TYPE_ERROR:
			icon = gtk_widget_render_icon(GTK_WIDGET(window->window), GTK_STOCK_DIALOG_ERROR, GTK_ICON_SIZE_MENU, NULL);
			break;
		case LOG_TYPE_WARNING:
			icon = gtk_widget_render_icon(GTK_WIDGET(window->window), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_MENU, NULL);
			break;
		case LOG_TYPE_INFO:
			icon = gtk_widget_render_icon(GTK_WIDGET(window->window), GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_MENU, NULL);
			break;
		case LOG_TYPE_DEBUG:
			icon = gtk_widget_render_icon(GTK_WIDGET(window->window), GTK_STOCK_INFO, GTK_ICON_SIZE_MENU, NULL);
			break;
	}

	logViewerAddMessage(window->logViewer, dateTime, message, icon);

	g_object_unref(icon);
	free(dateTime);
	free(now);
}

static gboolean closeWindow(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	HOOK_DETACH_EX(log, newLogMessage, data);

	LogViewerWindow *window = (LogViewerWindow *)data;
	freeLogViewerWindow(window);

	$$(void, exitGracefully)();

	return true;
}

static int cmpStringItems(const void *a, const void *b)
{
	const ConfigNodeValue *aItem = (const ConfigNodeValue*) a;
	const char *bItem = (const char *) b;

	return strcmp(aItem->content.string, bItem);
}

static gboolean addLogMessage(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	LOG_INFO("Hallo Welt");

	return true;
}
