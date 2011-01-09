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
#include <gtksourceview/gtksourceview.h>
#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagemanager.h>

#include "dll.h"
#include "modules/gtk+/gtk+.h"
#include "modules/gtk+/builder.h"
#include "modules/lang_lua/lang_lua.h"
#include "modules/module_util/module_util.h"
#include "api.h"

MODULE_NAME("lua_ide");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical Lua IDE using GTK+");
MODULE_VERSION(0, 3, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 2, 0), MODULE_DEPENDENCY("lang_lua", 0, 5, 2), MODULE_DEPENDENCY("module_util", 0, 1, 2));

/**
 * The GTK root widget for the IDE
 */
GtkWidget *window;

/**
 * The script input widget for the IDE
 */
GtkWidget *script_input;

/**
 * The console output widget for the IDE
 */
GtkWidget *console_output;

typedef enum {
	MESSAGE_ERR,
	MESSAGE_OUT
} MessageType;

static void runScript();
static void appendConsole(const char *message, MessageType type);

MODULE_INIT
{
	GString *path;

	path = g_string_new($$(char *, getExecutablePath)());
	g_string_append(path, "/modules/lua_ide/lua_ide.xml");

	GtkBuilder *builder = $(GtkBuilder *, gtk+, loadGtkBuilderGui)(path->str);

	g_string_free(path, true);

	if(builder == NULL) {
		LOG_ERROR("Failed to load Lua IDE GUI");
		return false;
	}

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	script_input = GTK_WIDGET(gtk_builder_get_object(builder, "script_input"));
	console_output = GTK_WIDGET(gtk_builder_get_object(builder, "console_output"));

	GtkRcStyle *style = gtk_widget_get_modifier_style(script_input);
	PangoFontDescription *font = pango_font_description_from_string("Monospace Normal");
	style->font_desc = font;
	gtk_widget_modify_style(script_input, style);

	style = gtk_widget_get_modifier_style(console_output);
	font = pango_font_description_from_string("Monospace Normal");
	style->font_desc = font;
	gtk_widget_modify_style(console_output, style);

	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

	GString *welcome = $$(GString *, dumpVersion)(&_module_version);
	g_string_prepend(welcome, "Welcome to the Kalisko Lua IDE ");
	g_string_append(welcome, "!");
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console_output));
	gtk_text_buffer_set_text(buffer, welcome->str, -1);
	g_string_free(welcome, true);

	gtk_window_set_title(GTK_WINDOW(window), "Kalisko Lua IDE");

	GtkSourceLanguageManager *manager = gtk_source_language_manager_get_default();
	GtkSourceLanguage *language = gtk_source_language_manager_get_language(manager, "lua");

	if(language != NULL) {
		GtkSourceBuffer *sbuffer = gtk_source_buffer_new_with_language(language);
		gtk_text_view_set_buffer(GTK_TEXT_VIEW(script_input), GTK_TEXT_BUFFER(sbuffer));
	} else {
		LOG_WARNING("Failed to set IDE editor language to lua");
	}

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

API gboolean lua_ide_window_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	$(void, module_util, safeRevokeModule)("lua_ide");
	return true;
}

API void lua_ide_menu_quit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	$(void, module_util, safeRevokeModule)("lua_ide");
}

API void lua_ide_menu_run_active(GtkMenuItem *menuitem, gpointer user_data)
{
	runScript();
}

API void lua_ide_run_button_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	runScript();
}

API void lua_ide_console_output_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console_output));
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(console_output), &end, 0.0, true, 1.0, 1.0);
}

static void runScript()
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(script_input));

	GtkTextIter start;
	gtk_text_buffer_get_start_iter(buffer, &start);
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);

	char *script = gtk_text_buffer_get_text(buffer, &start, &end, false);

	if(!$(bool, lang_lua, evaluateLua)(script)) {
		GString *err = g_string_new("Lua error: ");
		char *ret = $(char *, lang_lua, popLuaString)();
		g_string_append(err, ret);
		appendConsole(err->str, MESSAGE_ERR);
		free(ret);
		g_string_free(err, true);
	} else {
		char *ret = $(char *, lang_lua, popLuaString)();

		if(ret != NULL) {
			appendConsole(ret, MESSAGE_OUT);
			free(ret);
		}
	}

	free(script);
}

static void appendConsole(const char *message, MessageType type)
{
	// deactivate text view as long as we're writing
	gtk_widget_set_sensitive(console_output, FALSE);

	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console_output));

	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);

	GString *msg = g_string_new(message);
	g_string_prepend(msg, "\n");
	gtk_text_buffer_insert(buffer, &end, msg->str, -1);
	g_string_free(msg, true);

	gtk_widget_set_sensitive(console_output, TRUE);
}
