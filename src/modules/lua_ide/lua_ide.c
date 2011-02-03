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

#include <assert.h>
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
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "api.h"

MODULE_NAME("lua_ide");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical Lua IDE using GTK+");
MODULE_VERSION(0, 5, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 2, 0), MODULE_DEPENDENCY("lua", 0, 8, 0), MODULE_DEPENDENCY("module_util", 0, 1, 2), MODULE_DEPENDENCY("store", 0, 6, 10), MODULE_DEPENDENCY("config", 0, 3, 9));

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

/**
 * The script tree widget for the IDE
 */
static GtkWidget *script_tree;

/**
 * The IDE's config store
 */
static Store *ide_config;

/**
 * The currently opened script
 */
static char *current_script = NULL;

/**
 * True if the currently opened script has changed since last saving
 */
static bool script_changed = false;

typedef enum {
	MESSAGE_LUA_ERR,
	MESSAGE_LUA_OUT,
	MESSAGE_OUT
} MessageType;

typedef enum {
   SCRIPT_TREE_NAME_COLUMN,
   SCRIPT_TREE_TYPE_COLUMN,
   SCRIPT_TREE_PATH_COLUMN
} ScriptTreeColumns;

static void lua_ide_script_input_buffer_changed(GtkTextBuffer *textbuffer, gpointer user_data);
static void finalize();
static void runScript();
static void appendConsole(const char *message, MessageType type);
static int lua_output(lua_State *state);
static void undo();
static void redo();
static void clearOutput();
static void fillScriptTreeStore(GtkTreeStore *treestore, GtkTreeIter *parent, GString *path, Store *files);
static bool openScript(char *path);
static void refreshScriptTree();
static void refreshWindowTitle();

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
	script_tree = GTK_WIDGET(gtk_builder_get_object(builder, "script_tree"));

	// script tree
	Store *config = $(Store *, config, getWritableConfig)();
	if((ide_config = $(Store *, store, getStorePath)(config, "lua_ide")) == NULL) {
		LOG_INFO("Writable config path 'lua_ide' doesn't exist yet, creating...");
		ide_config = $(Store *, store, createStore)();
		$(bool, store, setStorePath)(config, "lua_ide", ide_config);
	}

	refreshScriptTree();

	GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", SCRIPT_TREE_NAME_COLUMN, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(script_tree), column);

	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(script_tree));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

	// script input
	GtkRcStyle *style = gtk_widget_get_modifier_style(script_input);
	PangoFontDescription *font = pango_font_description_from_string("Monospace Normal");
	style->font_desc = font;
	gtk_widget_modify_style(script_input, style);

	GtkSourceLanguageManager *manager = gtk_source_language_manager_get_default();
	GtkSourceLanguage *language = gtk_source_language_manager_get_language(manager, "lua");

	if(language != NULL) {
		GtkSourceBuffer *sbuffer = gtk_source_buffer_new_with_language(language);
		g_signal_connect(G_OBJECT(sbuffer), "changed", G_CALLBACK(lua_ide_script_input_buffer_changed), NULL);
		gtk_text_view_set_buffer(GTK_TEXT_VIEW(script_input), GTK_TEXT_BUFFER(sbuffer));
	} else {
		LOG_WARNING("Failed to set IDE editor language to lua");
	}

	// window
	gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);

	// console output
	style = gtk_widget_get_modifier_style(console_output);
	font = pango_font_description_from_string("Monospace Normal");
	style->font_desc = font;
	gtk_widget_modify_style(console_output, style);

	GString *welcome = $$(GString *, dumpVersion)(&_module_version);
	g_string_prepend(welcome, "Welcome to the Kalisko Lua IDE ");
	g_string_append(welcome, "!");
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console_output));
	gtk_text_buffer_set_text(buffer, welcome->str, -1);
	g_string_free(welcome, true);
	gtk_text_buffer_create_tag(buffer, "lua_error", "foreground", "red", "weight", PANGO_WEIGHT_BOLD, NULL);
	gtk_text_buffer_create_tag(buffer, "lua_out", "foreground", "blue", NULL);

	// show everything
	gtk_widget_show_all(GTK_WIDGET(window));

	// run
	$(void, gtk+, runGtkLoop)();

	// Lua C function
	lua_State *state = $(lua_State *, lua, getGlobalLuaState)();
	lua_pushcfunction(state, &lua_output);
	lua_setglobal(state, "output");

	if(!openScript("scripts/default")) {
		finalize();
		return false;
	}

	return true;
}

MODULE_FINALIZE
{
	finalize();
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

API void lua_ide_script_tree_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	GtkTreeSelection *select = gtk_tree_view_get_selection(tree_view);
    GtkTreeIter iter;
    GtkTreeModel *model;

    if(gtk_tree_selection_get_selected(select, &model, &iter)) {
    	int type;
    	char *path;
    	gtk_tree_model_get(model, &iter, SCRIPT_TREE_TYPE_COLUMN, &type, SCRIPT_TREE_PATH_COLUMN, &path, -1);

    	if(type == 1) {
    		openScript(path);
    	}
    }
}

API void lua_ide_console_output_size_allocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data)
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(console_output));
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(console_output), &end, 0.0, true, 1.0, 1.0);
}

static void lua_ide_script_input_buffer_changed(GtkTextBuffer *textbuffer, gpointer user_data)
{
	script_changed = true;
	refreshWindowTitle();
}

static void finalize()
{
	lua_State *state = $(lua_State *, lua, getGlobalLuaState)();
	lua_pushnil(state);
	lua_setglobal(state, "output");

	gtk_widget_destroy(GTK_WIDGET(window));
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

	GDateTime *now = g_date_time_new_now_local();
	GString *prefix = g_string_new("\n");
	g_string_append_printf(prefix, "[%02d:%02d:%02d] ", g_date_time_get_hour(now), g_date_time_get_minute(now), g_date_time_get_second(now));
	g_date_time_unref(now);
	gtk_text_buffer_insert(buffer, &end, prefix->str, -1);
	g_string_free(prefix, true);

	switch(type) {
		case MESSAGE_LUA_ERR:
			gtk_text_buffer_insert_with_tags_by_name(buffer, &end, message, -1, "lua_error", NULL);
		break;
		case MESSAGE_LUA_OUT:
			gtk_text_buffer_insert_with_tags_by_name(buffer, &end, message, -1, "lua_out", NULL);
		break;
		case MESSAGE_OUT:
			gtk_text_buffer_insert(buffer, &end, message, -1);
		break;
	}

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

static void fillScriptTreeStore(GtkTreeStore *treestore, GtkTreeIter *parent, GString *path, Store *scripts)
{
	assert(scripts->type == STORE_ARRAY);

	GHashTableIter iter;
	char *key;
	Store *value;

	g_hash_table_iter_init(&iter, scripts->content.array);
	while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
		GtkTreeIter child;
		gtk_tree_store_append(treestore, &child, parent);

		GString *subpath = g_string_new(path->str);
		g_string_append_printf(subpath, "/%s", key);
		switch(value->type) {
			case STORE_ARRAY:
				gtk_tree_store_set(treestore, &child, SCRIPT_TREE_NAME_COLUMN, key, SCRIPT_TREE_TYPE_COLUMN, 0, SCRIPT_TREE_PATH_COLUMN, subpath->str, -1);

				do {
					fillScriptTreeStore(treestore, &child, subpath, value);
				} while(false);
			break;
			case STORE_STRING:
				gtk_tree_store_set(treestore, &child, SCRIPT_TREE_NAME_COLUMN, key, SCRIPT_TREE_TYPE_COLUMN, 1, SCRIPT_TREE_PATH_COLUMN, subpath->str, -1);
			break;
			default:
				LOG_WARNING("Config store value in '%s' has unsupported type when filling Lua IDE script tree, expected array or string - skipping...", subpath->str);
			break;
		}
		g_string_free(subpath, true);
	}
}

static bool openScript(char *path)
{
	Store *script = $(Store *, store, getStorePath)(ide_config, path);

	if(script != NULL) {
		LOG_INFO("Opening script: %s", path);

		if(script->type == STORE_STRING) {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(script_input));

			GtkTextIter start;
			gtk_text_buffer_get_start_iter(buffer, &start);
			GtkTextIter end;
			gtk_text_buffer_get_end_iter(buffer, &end);

			gtk_text_buffer_delete(buffer, &start, &end);

			gtk_text_buffer_insert(buffer, &start, script->content.string, -1);
		} else {
			LOG_ERROR("Lua IDE config store path '%s' is not a string, aborting script opening", path);
			return false;
		}
	} else {
		LOG_INFO("Creating script: %s", path);

		GPtrArray *path_parts = $(GPtrArray *, store, splitStorePath)(path);
		Store *last = ide_config;

		for(int i = 0; i < path_parts->len; i++) {
			char *part = path_parts->pdata[i];

			if(i != path_parts->len - 1) {
				Store *next = $(Store *, store, getStorePath)(last, part);

				if(next == NULL) {
					next = $(Store *, store, createStore)();
					$(bool, store, setStorePath)(last, part, next);
				} else if(next->type != STORE_ARRAY) {
					LOG_ERROR("Lua IDE config store part '%s' in path '%s' is not an array, aborting script creation", part, path);
					g_ptr_array_free(path_parts, true);
					return false;
				}

				last = next;
			}
		}

		$(bool, store, setStorePath)(last, path_parts->pdata[path_parts->len-1], $(Store *, store, createStoreStringValue)(""));

		refreshScriptTree();
	}

	if(current_script != NULL) {
		free(current_script);
	}

	current_script = strdup(path);
	script_changed = false;
	refreshWindowTitle();

	return true;
}

static void refreshScriptTree()
{
	GtkTreeStore *treestore = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(script_tree)));
	gtk_tree_store_clear(treestore);

	Store *scripts;
	if((scripts = $(Store *, store, getStorePath)(ide_config, "scripts")) == NULL) {
		LOG_INFO("Lua IDE config store path 'scripts' doesn't exist yet, creating...");
		scripts = $(Store *, store, createStore)();
		$(bool, store, setStorePath)(ide_config, "scripts", scripts);
	}

	GString *path = g_string_new("scripts");
	fillScriptTreeStore(treestore, NULL, path, scripts);
	g_string_free(path, true);
}

static void refreshWindowTitle()
{
	GString *title = g_string_new("Kalisko Lua IDE - ");

	if(script_changed) {
		g_string_append(title, "*");
	}

	g_string_append(title, current_script);
	gtk_window_set_title(GTK_WINDOW(window), title->str);
	g_string_free(title, true);
}
