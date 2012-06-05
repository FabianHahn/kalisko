/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2012, Kalisko Project Leaders
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
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#define API

MODULE_NAME("lua_ide");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical Lua IDE using GTK+");
MODULE_VERSION(0, 9, 11);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 2, 6), MODULE_DEPENDENCY("gtksourceview", 0, 1, 0), MODULE_DEPENDENCY("lua", 0, 8, 0), MODULE_DEPENDENCY("store", 0, 6, 12), MODULE_DEPENDENCY("config", 0, 3, 9), MODULE_DEPENDENCY("xcall_core", 0, 4, 3));

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
 * The script tree context menu folder widget for the IDE
 */
static GtkWidget *script_tree_context_menu_folder;

/**
 * The script tree context menu script widget for the IDE
 */
static GtkWidget *script_tree_context_menu_script;

/**
 * The script tree context menu blank widget for the IDE
 */
static GtkWidget *script_tree_context_menu_blank;

/**
 * The text input dialog widget for the IDE
 */
static GtkWidget *text_input_dialog;

/**
 * The text input dialog label widget for the IDE
 */
static GtkWidget *text_input_dialog_label;

/**
 * The text input dialog entry widget for the IDE
 */
static GtkWidget *text_input_dialog_entry;

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

/**
 * The path to the currently selected tree element when a context menu is open
 */
static GtkTreePath *tree_path = NULL;

typedef enum {
	MESSAGE_LUA_ERR,
	MESSAGE_LUA_OUT,
	MESSAGE_OUT
} MessageType;

typedef enum {
	SCRIPT_TREE_NAME_COLUMN,
	SCRIPT_TREE_TYPE_COLUMN,
	SCRIPT_TREE_PATH_COLUMN,
	SCRIPT_TREE_ICON_COLUMN
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
static void saveScript();
static void createScript(char *parent);
static void createFolder(char *parent);
static int strpcmp(const void *p1, const void *p2);
static void deleteScript(char *script);
static void deleteFolder(char *folder);

MODULE_INIT
{
	GString *path = g_string_new($$(char *, getExecutablePath)());
	g_string_append(path, "/modules/lua_ide/lua_ide.xml");

	GtkBuilder *builder = loadGtkBuilderGui(path->str);

	g_string_free(path, true);

	if(builder == NULL) {
		LOG_ERROR("Failed to load Lua IDE GUI");
		return false;
	}

	window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
	script_input = GTK_WIDGET(gtk_builder_get_object(builder, "script_input"));
	console_output = GTK_WIDGET(gtk_builder_get_object(builder, "console_output"));
	script_tree = GTK_WIDGET(gtk_builder_get_object(builder, "script_tree"));
	script_tree_context_menu_folder = GTK_WIDGET(gtk_builder_get_object(builder, "script_tree_context_menu_folder"));
	script_tree_context_menu_script = GTK_WIDGET(gtk_builder_get_object(builder, "script_tree_context_menu_script"));
	script_tree_context_menu_blank = GTK_WIDGET(gtk_builder_get_object(builder, "script_tree_context_menu_blank"));
	text_input_dialog = GTK_WIDGET(gtk_builder_get_object(builder, "text_input_dialog"));
	text_input_dialog_label = GTK_WIDGET(gtk_builder_get_object(builder, "text_input_dialog_label"));
	text_input_dialog_entry = GTK_WIDGET(gtk_builder_get_object(builder, "text_input_dialog_entry"));

	// script tree
	Store *config = getWritableConfig();
	if((ide_config = getStorePath(config, "lua_ide")) == NULL) {
		LOG_INFO("Writable config path 'lua_ide' doesn't exist yet, creating...");
		ide_config = createStore();
		setStorePath(config, "lua_ide", ide_config);
	}

	refreshScriptTree();

	GtkCellRenderer *rendererPixbuf = gtk_cell_renderer_pixbuf_new();
	GtkCellRenderer *rendererText = gtk_cell_renderer_text_new();
	GtkTreeViewColumn *column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, rendererPixbuf, false);
	gtk_tree_view_column_add_attribute(column, rendererPixbuf, "stock-id", SCRIPT_TREE_ICON_COLUMN);
	gtk_tree_view_column_pack_start(column, rendererText, true);
	gtk_tree_view_column_add_attribute(column, rendererText, "text", SCRIPT_TREE_NAME_COLUMN);
	gtk_tree_view_append_column(GTK_TREE_VIEW(script_tree), column);

	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(script_tree));
	gtk_tree_selection_set_mode(select, GTK_SELECTION_SINGLE);

	g_object_set(G_OBJECT(script_tree), "headers_visible", false, NULL);

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
	runGtkLoop();

	// Lua C function
	lua_State *state = getGlobalLuaState();
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
	$$(void, exitGracefully)();
	return true;
}

API void lua_ide_menu_quit_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	$$(void, exitGracefully)();
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

API void lua_ide_save_button_clicked(GtkToolButton *toolbutton, gpointer user_data)
{
	saveScript();
}

API void lua_ide_menu_save_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	saveScript();
}

API void lua_ide_script_tree_context_menu_script_open_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(tree_path != NULL) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(script_tree));
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, tree_path);
		int type;
		char *path;
		gtk_tree_model_get(model, &iter, SCRIPT_TREE_TYPE_COLUMN, &type, SCRIPT_TREE_PATH_COLUMN, &path, -1);

		if(type == 1) {
			openScript(path); // open the script
		}

		gtk_tree_path_free(tree_path); // not used anymore
		tree_path = NULL;
	}
}

API void lua_ide_script_tree_context_menu_folder_toggle_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(tree_path != NULL) {
		if(gtk_tree_view_row_expanded(GTK_TREE_VIEW(script_tree), tree_path)) { // check if expanded or collapsed
			gtk_tree_view_collapse_row(GTK_TREE_VIEW(script_tree), tree_path);
		} else {
			gtk_tree_view_expand_row(GTK_TREE_VIEW(script_tree), tree_path, false);
		}
		gtk_tree_path_free(tree_path); // not used anymore
		tree_path = NULL;
	}
}

API void lua_ide_script_tree_context_menu_blank_new_folder_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	createFolder("scripts");
}

API void lua_ide_script_tree_context_menu_blank_new_script_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	createScript("scripts");
}

API void lua_ide_script_tree_context_menu_folder_new_folder_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(tree_path != NULL) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(script_tree));
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, tree_path);
		int type;
		char *path;
		gtk_tree_model_get(model, &iter, SCRIPT_TREE_TYPE_COLUMN, &type, SCRIPT_TREE_PATH_COLUMN, &path, -1);

		if(type == 0) {
			createFolder(path);
		}

		gtk_tree_path_free(tree_path); // not used anymore
		tree_path = NULL;
	}
}

API void lua_ide_script_tree_context_menu_folder_new_script_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(tree_path != NULL) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(script_tree));
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, tree_path);
		int type;
		char *path;
		gtk_tree_model_get(model, &iter, SCRIPT_TREE_TYPE_COLUMN, &type, SCRIPT_TREE_PATH_COLUMN, &path, -1);

		if(type == 0) {
			createScript(path);
		}

		gtk_tree_path_free(tree_path); // not used anymore
		tree_path = NULL;
	}
}

API void lua_ide_script_tree_context_menu_script_delete_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(tree_path != NULL) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(script_tree));
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, tree_path);
		int type;
		char *path;
		gtk_tree_model_get(model, &iter, SCRIPT_TREE_TYPE_COLUMN, &type, SCRIPT_TREE_PATH_COLUMN, &path, -1);

		if(type == 1) {
			deleteScript(path); // open the script
		}

		gtk_tree_path_free(tree_path); // not used anymore
		tree_path = NULL;
	}
}

API void lua_ide_script_tree_context_menu_folder_delete_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	if(tree_path != NULL) {
		GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(script_tree));
		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, tree_path);
		int type;
		char *path;
		gtk_tree_model_get(model, &iter, SCRIPT_TREE_TYPE_COLUMN, &type, SCRIPT_TREE_PATH_COLUMN, &path, -1);

		if(type == 0) {
			deleteFolder(path);
		}

		gtk_tree_path_free(tree_path); // not used anymore
		tree_path = NULL;
	}
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

    	if(type == 0) {
    		GtkTreePath *tpath = gtk_tree_model_get_path(model, &iter);
    		if(gtk_tree_view_row_expanded(GTK_TREE_VIEW(script_tree), tpath)) { // check if expanded or collapsed
    			gtk_tree_view_collapse_row(GTK_TREE_VIEW(script_tree), tpath);
    		} else {
    			gtk_tree_view_expand_row(GTK_TREE_VIEW(script_tree), tpath, false);
    		}
    		gtk_tree_path_free(tpath);
    	} else {
    		openScript(path);
    	}
    }
}

API bool lua_ide_script_tree_button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
	GtkMenu *menu = NULL;

	GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(script_tree));

	if(tree_path != NULL) { // if the tree path still exists, free it first to avoid leaks
		gtk_tree_path_free(tree_path);
	}

	gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(script_tree), event->x, event->y, &tree_path, NULL, NULL, NULL);

	GtkTreeSelection *select = gtk_tree_view_get_selection(GTK_TREE_VIEW(script_tree));
	if(tree_path != NULL) {
		gtk_tree_selection_select_path(select, tree_path); // make sure clicked element is selected visually

		GtkTreeIter iter;
		gtk_tree_model_get_iter(model, &iter, tree_path);
		int type;
		gtk_tree_model_get(model, &iter, SCRIPT_TREE_TYPE_COLUMN, &type, -1);

		if(type == 0) { // show folder menu
			menu = GTK_MENU(script_tree_context_menu_folder);
		} else { // show script menu
			menu = GTK_MENU(script_tree_context_menu_script);
		}
	} else { // show blank menu
		gtk_tree_selection_unselect_all(select);
		menu = GTK_MENU(script_tree_context_menu_blank);
	}

	if(event->button == 3) {
		gtk_menu_popup(menu, NULL, NULL, NULL, NULL, event->button, event->time);
		return true;
	}

	return false;
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
	if(tree_path != NULL) {
		gtk_tree_path_free(tree_path);
	}

	if(current_script != NULL) {
		free(current_script);
	}

	lua_State *state = getGlobalLuaState();
	lua_pushnil(state);
	lua_setglobal(state, "output");

	gtk_widget_destroy(GTK_WIDGET(window));
	gtk_widget_destroy(GTK_WIDGET(text_input_dialog));
}

static void runScript()
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(script_input));

	GtkTextIter start;
	gtk_text_buffer_get_start_iter(buffer, &start);
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);

	char *script = gtk_text_buffer_get_text(buffer, &start, &end, false);

	if(!evaluateLua(script)) {
		GString *err = g_string_new("Lua error: ");
		char *ret = popLuaString();
		g_string_append(err, ret);
		appendConsole(err->str, MESSAGE_LUA_ERR);
		free(ret);
		g_string_free(err, true);
	} else {
		char *ret = popLuaString();

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

	// create folder and script arrays
	GPtrArray *folderEntries = g_ptr_array_new();
	GPtrArray *scriptEntries = g_ptr_array_new();
	while(g_hash_table_iter_next(&iter, (void *) &key, (void *) &value)) {
		if(value->type == STORE_ARRAY) {
			g_ptr_array_add(folderEntries, key);
		} else if(value->type == STORE_STRING) {
			g_ptr_array_add(scriptEntries, key);
		}
	}

	// sort them seperately
	qsort(folderEntries->pdata, folderEntries->len, sizeof(char *), &strpcmp);
	qsort(scriptEntries->pdata, scriptEntries->len, sizeof(char *), &strpcmp);

	// add sorted folders first
	for(int i = 0; i < folderEntries->len; i++) {
		key = folderEntries->pdata[i] ;
		Store *value = g_hash_table_lookup(scripts->content.array, key);

		GtkTreeIter child;
		gtk_tree_store_append(treestore, &child, parent);

		GString *subpath = g_string_new(path->str);
		g_string_append_printf(subpath, "/%s", key);
		gtk_tree_store_set(treestore, &child, SCRIPT_TREE_NAME_COLUMN, key, SCRIPT_TREE_TYPE_COLUMN, 0, SCRIPT_TREE_PATH_COLUMN, subpath->str, SCRIPT_TREE_ICON_COLUMN, GTK_STOCK_DIRECTORY, -1);
		fillScriptTreeStore(treestore, &child, subpath, value);
		g_string_free(subpath, true);
	}

	// then add sorted scripts
	for(int i = 0; i < scriptEntries->len; i++) {
		key = scriptEntries->pdata[i] ;

		GtkTreeIter child;
		gtk_tree_store_append(treestore, &child, parent);

		GString *subpath = g_string_new(path->str);
		g_string_append_printf(subpath, "/%s", key);
		gtk_tree_store_set(treestore, &child, SCRIPT_TREE_NAME_COLUMN, key, SCRIPT_TREE_TYPE_COLUMN, 1, SCRIPT_TREE_PATH_COLUMN, subpath->str, SCRIPT_TREE_ICON_COLUMN, GTK_STOCK_FILE, -1);
		g_string_free(subpath, true);
	}

	g_ptr_array_free(folderEntries, true);
	g_ptr_array_free(scriptEntries, true);
}

static bool openScript(char *path)
{
	if(current_script != NULL && script_changed) {
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "The currently opened script '%s' contains unsaved changes.\nDo you want to save it first?", current_script);
		gtk_dialog_add_buttons(GTK_DIALOG(dialog), "_Cancel", 0, "_Don't save", 1, "_Save", 2, NULL);
		int result = gtk_dialog_run(GTK_DIALOG(dialog));

		if(result == 2) {
			saveScript();
		}

		gtk_widget_destroy(dialog);

		if(result == 0) { // Abort selected, don't open new file
			return false;
		}
	}

	Store *script = getStorePath(ide_config, "%s", path);

	if(script != NULL) {
		if(script->type == STORE_STRING) {
			GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(script_input));

			GtkTextIter start;
			gtk_text_buffer_get_start_iter(buffer, &start);
			GtkTextIter end;
			gtk_text_buffer_get_end_iter(buffer, &end);

			gtk_text_buffer_delete(buffer, &start, &end);

			gtk_text_buffer_insert(buffer, &start, script->content.string, -1);

			LOG_INFO("Opened Lua IDE script: %s", path);
		} else {
			LOG_ERROR("Lua IDE config store path '%s' is not a string, aborting script opening", path);
			return false;
		}
	} else {
		GPtrArray *path_parts = splitStorePath(path);
		Store *last = ide_config;

		for(int i = 0; i < path_parts->len; i++) {
			char *part = path_parts->pdata[i];

			if(i != path_parts->len - 1) {
				Store *next = getStorePath(last, "%s", part);

				if(next == NULL) {
					next = createStore();
					setStorePath(last, "%s", next, part);
				} else if(next->type != STORE_ARRAY) {
					LOG_ERROR("Lua IDE config store part '%s' in path '%s' is not an array, aborting script creation", part, path);
					g_ptr_array_free(path_parts, true);
					return false;
				}

				last = next;
			}
		}

		setStorePath(last, "%s", createStoreStringValue(""), path_parts->pdata[path_parts->len-1]);

		saveWritableConfig(); // write back to disk
		refreshScriptTree();

		GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(script_input));
		GtkTextIter start;
		gtk_text_buffer_get_start_iter(buffer, &start);
		GtkTextIter end;
		gtk_text_buffer_get_end_iter(buffer, &end);

		gtk_text_buffer_delete(buffer, &start, &end);

		LOG_INFO("Created Lua IDE script: %s", path);
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
	if((scripts = getStorePath(ide_config, "scripts")) == NULL) {
		LOG_INFO("Lua IDE config store path 'scripts' doesn't exist yet, creating...");
		scripts = createStore();
		setStorePath(ide_config, "scripts", scripts);
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

	if(current_script != NULL) {
		g_string_append(title, current_script);
	}

	gtk_window_set_title(GTK_WINDOW(window), title->str);
	g_string_free(title, true);
}

static void saveScript()
{
	GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(script_input));

	GtkTextIter start;
	gtk_text_buffer_get_start_iter(buffer, &start);
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(buffer, &end);

	char *script = gtk_text_buffer_get_text(buffer, &start, &end, false);

	Store *store = getStorePath(ide_config, "%s", current_script);

	if(store != NULL && store->type == STORE_STRING) {
		free(store->content.string);
		store->content.string = script;
		saveWritableConfig(); // write back to disk
		LOG_INFO("Saved Lua IDE script: %s", current_script);
	} else {
		LOG_WARNING("Failed to save script '%s' to Lua IDE config store", current_script);
		free(script);
	}

	script_changed = false;
	refreshWindowTitle();
}

static void createScript(char *parent)
{
	GtkDialog *dialog = GTK_DIALOG(text_input_dialog);
	gtk_window_set_title(GTK_WINDOW(dialog), "Enter name");
	gtk_label_set_text(GTK_LABEL(text_input_dialog_label), "Please enter the name of the script to create:");
	gtk_entry_set_text(GTK_ENTRY(text_input_dialog_entry), "");
	int result = gtk_dialog_run(dialog);
	gtk_widget_hide(GTK_WIDGET(dialog));

	if(result == 1) {
		const char *entry_name = gtk_entry_get_text(GTK_ENTRY(text_input_dialog_entry));
		if(entry_name != NULL && strlen(entry_name) > 0) {
			GString *newpath = g_string_new(parent);
			g_string_append_printf(newpath, "/%s", entry_name);
			openScript(newpath->str);
			g_string_free(newpath, true);
		}
	}
}

static void createFolder(char *parent)
{
	Store *parentStore = getStorePath(ide_config, "%s", parent);

	if(parentStore != NULL && parentStore->type == STORE_ARRAY) {
		GtkDialog *dialog = GTK_DIALOG(text_input_dialog);
		gtk_window_set_title(GTK_WINDOW(dialog), "Enter name");
		gtk_label_set_text(GTK_LABEL(text_input_dialog_label), "Please enter the name of the folder to create:");
		gtk_entry_set_text(GTK_ENTRY(text_input_dialog_entry), "");
		int result = gtk_dialog_run(dialog);
		gtk_widget_hide(GTK_WIDGET(dialog));

		if(result == 1) {
			const char *entry_name = gtk_entry_get_text(GTK_ENTRY(text_input_dialog_entry));
			if(entry_name != NULL && strlen(entry_name) > 0) {
				if(getStorePath(parentStore, "%s", entry_name) == NULL) { // entry with that name doesn't exist yet
					LOG_INFO("Created Lua IDE folder '%s' in '%s'", entry_name, parent);
					g_hash_table_insert(parentStore->content.array, strdup(entry_name), createStore());
					saveWritableConfig(); // write back to disk
					refreshScriptTree();
				} else {
					LOG_ERROR("Tried to create Lua IDE folder with already existing name '%s' in '%s', aborting", entry_name, parent);
				}
			}
		}
	} else {
		LOG_ERROR("Failed to create Lua IDE folder in parent config store path '%s': Not a store array", parent);
	}
}

static void deleteScript(char *script)
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "Do you really want to delete the script '%s'?", script);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), "_Cancel", 0, "_Delete", 1, NULL);
	int result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if(result != 1) { // Abort selected, don't delete
		return;
	}

	deleteStorePath(ide_config, script);
	saveWritableConfig(); // write back to disk
	refreshScriptTree();
	LOG_INFO("Deleted Lua IDE script: %s", script);

	if(g_strcmp0(script, current_script) == 0) {
		script_changed = false;
		openScript("scripts/default");
	}
}

static void deleteFolder(char *folder)
{
	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_NONE, "Do you really want to delete the folder '%s' with all its contents?", folder);
	gtk_dialog_add_buttons(GTK_DIALOG(dialog), "_Cancel", 0, "_Delete", 1, NULL);
	int result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if(result != 1) { // Abort selected, don't delete
		return;
	}

	deleteStorePath(ide_config, folder);
	saveWritableConfig(); // write back to disk
	refreshScriptTree();
	LOG_INFO("Deleted Lua IDE folder: %s", folder);
}

static int strpcmp(const void *p1, const void *p2)
{
	return strcmp(* (char * const *) p1, * (char * const *) p2);
}

