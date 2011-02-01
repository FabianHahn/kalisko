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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "dll.h"
#include "modules/gtk+/gtk+.h"
#include "modules/gtk+/builder.h"
#include "modules/lua/module_lua.h"
#include "modules/module_util/module_util.h"
#include "api.h"

MODULE_NAME("lua_ide");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical Lua IDE using GTK+");
MODULE_VERSION(0, 4, 5);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 2, 0), MODULE_DEPENDENCY("lua", 0, 8, 0), MODULE_DEPENDENCY("module_util", 0, 1, 2));

/**
 * The GTK root widget for the IDE
 */
static GtkWidget *window;

/**
 * The script input widget for the IDE
 */
static GtkWidget *script_input;

/**
 * The console output widget for the IDE
 */
static GtkWidget *console_output;

typedef enum {
	MESSAGE_LUA_ERR,
	MESSAGE_LUA_OUT,
	MESSAGE_OUT
} MessageType;

static void runScript();
static void appendConsole(const char *message, MessageType type);
static int lua_output(lua_State *state);
static void undo();
static void redo();
static void clearOutput();

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
	gtk_text_buffer_create_tag(buffer, "lua_error", "foreground", "red", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag(buffer, "lua_out", "foreground", "blue", NULL);

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

	lua_State *state = $(lua_State *, lua, getGlobalLuaState)();
	lua_pushcfunction(state, &lua_output);
	lua_setglobal(state, "output");

	return true;
}

MODULE_FINALIZE
{
	lua_State *state = $(lua_State *, lua, getGlobalLuaState)();
	lua_pushnil(state);
	lua_setglobal(state, "output");

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

API void lua_ide_menu_run_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	runScript();
}

API void lua_ide_run_button_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	runScript();
}

API void lua_ide_undo_button_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	undo();
}

API void lua_ide_redo_button_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	redo();
}

API void lua_ide_menu_undo_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	undo();
}

API void lua_ide_menu_redo_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	redo();
}

API void lua_ide_clear_button_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	clearOutput();
}

API void lua_ide_menu_clear_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	clearOutput();
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

	if(!$(bool, lua, evaluateLua)(script)) {
		GString *err = g_string_new("Lua error: ");
		char *ret = $(char *, lua, popLuaString)();
		g_string_append(err, ret);
		appendConsole(err->str, MESSAGE_LUA_ERR);
		free(ret);
		g_string_free(err, true);
	} else {
		char *ret = $(char *, lua, popLuaString)();

		if(ret != NULL) {
			appendConsole(ret, MESSAGE_LUA_OUT);
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

	switch(type) {
		case MESSAGE_LUA_ERR:
			gtk_text_buffer_insert_with_tags_by_name(buffer, &end, msg->str, -1, "lua_error", NULL);
		break;
		case MESSAGE_LUA_OUT:
			gtk_text_buffer_insert_with_tags_by_name(buffer, &end, msg->str, -1, "lua_out", NULL);
		break;
		case MESSAGE_OUT:
			gtk_text_buffer_insert(buffer, &end, msg->str, -1);
		break;
	}

	g_string_free(msg, true);

	gtk_widget_set_sensitive(console_output, TRUE);
}

static int lua_output(lua_State *state)
{
	const char *string = luaL_checkstring(state, 1);

	appendConsole(string, MESSAGE_OUT);

	return 0;
}

static void undo()
{
	GtkSourceBuffer *sbuffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(script_input)));

	if(gtk_source_buffer_can_undo(sbuffer)) {
		gtk_source_buffer_undo(sbuffer);
	}
}

static void redo()
{
	GtkSourceBuffer *sbuffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(script_input)));

	if(gtk_source_buffer_can_redo(sbuffer)) {
		gtk_source_buffer_redo(sbuffer);
	}
}

static void clearOutput()
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console_output));

	GtkTextIter start;
	gtk_text_buffer_get_start_iter(buffer, &start);
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);

	gtk_text_buffer_delete(buffer, &start, &end);
}
