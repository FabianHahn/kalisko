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
#include "modules/gtk+/gtk+.h"
#include "modules/gtk+/builder.h"
#include "modules/lang_lua/lang_lua.h"
#include "api.h"

MODULE_NAME("lua_ide");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A graphical Lua IDE using GTK+");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("gtk+", 0, 2, 0), MODULE_DEPENDENCY("lang_lua", 0, 5, 2));

/**
 * The GTK root widget for the IDE
 */
GtkWidget *window;

MODULE_INIT
{
	GString *path;

	path = g_string_new($$(char *, getExecutablePath)());
	g_string_append(path, "/modules/lua_ide/lua_ide.xml");

	window = $(GtkWidget *, gtk+, loadGtkBuilderGui)(path->str, "window");

	g_string_free(path, true);

	if(window == NULL) {
		LOG_ERROR("Failed to load Lua IDE GUI");
		return false;
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
