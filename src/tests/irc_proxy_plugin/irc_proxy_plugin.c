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

#include <glib.h>
#include <string.h>

#include "dll.h"
#include "test.h"
#include "memory_alloc.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/irc_proxy_plugin/irc_proxy_plugin.h"
#include "api.h"

MODULE_NAME("test_irc_proxy_plugin");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the irc_proxy_plugin module");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc_proxy_plugin", 0, 1, 0), MODULE_DEPENDENCY("irc_proxy", 0, 1, 13));

TEST_CASE(plugin_add);
TEST_CASE(plugin_use);
TEST_CASE(plugin_del);

static IrcProxyPlugin plugin;

static IrcProxy *createProxyStub();
static void freeProxyStub(IrcProxy *proxy);

TEST_SUITE_BEGIN(irc_proxy_plugin)
	TEST_CASE_ADD(plugin_add);
	TEST_CASE_ADD(plugin_use);
	TEST_CASE_ADD(plugin_del);
TEST_SUITE_END

TEST_CASE(plugin_add)
{
	plugin.handlers = g_queue_new();
	plugin.name = "testplugin";

	TEST_ASSERT($(bool, irc_proxy_plugin, addIrcProxyPlugin)(&plugin));

	TEST_PASS;
}

TEST_CASE(plugin_use)
{
	IrcProxy *proxy = createProxyStub();

	// Test if plugin enable and disable fails on non-enabled proxy
	TEST_ASSERT(!$(bool, irc_proxy_plugin, enableIrcProxyPlugin)(proxy, "testplugin"));
	TEST_ASSERT(!$(bool, irc_proxy_plugin, disableIrcProxyPlugin)(proxy, "testplugin"));

	// enable our stub proxy
	TEST_ASSERT($(bool, irc_proxy_plugin, enableIrcProxyPlugins)(proxy));

	TEST_ASSERT(!$(bool, irc_proxy_plugin, disableIrcProxyPlugin)(proxy, "testplugin")); // disable should fail if not loaded
	TEST_ASSERT($(bool, irc_proxy_plugin, enableIrcProxyPlugin)(proxy, "testplugin")); // enable test plugin
	TEST_ASSERT(!$(bool, irc_proxy_plugin, enableIrcProxyPlugin)(proxy, "testplugin")); // enable again should fail
	TEST_ASSERT($(bool, irc_proxy_plugin, disableIrcProxyPlugin)(proxy, "testplugin")); // disable test plugin

	TEST_ASSERT(!$(bool, irc_proxy_plugin, enableIrcProxyPlugin)(proxy, "not existing plugin")); // not existing plugin cannot be enabled

	// disable our stub proxy
	$(void, irc_proxy_plugin, disableIrcProxyPlugins)(proxy);

	// Test (again) if plugin enable and disable fails on non-enabled proxy
	TEST_ASSERT(!$(bool, irc_proxy_plugin, enableIrcProxyPlugin)(proxy, "testplugin"));
	TEST_ASSERT(!$(bool, irc_proxy_plugin, disableIrcProxyPlugin)(proxy, "testplugin"));

	freeProxyStub(proxy);

	TEST_PASS;
}

TEST_CASE(plugin_del)
{
	$(void, irc_proxy_plugin, delIrcProxyPlugin)(&plugin);
	TEST_ASSERT(g_queue_get_length(plugin.handlers) == 0);

	g_queue_free(plugin.handlers);

	TEST_PASS;
}

static IrcProxy *createProxyStub()
{
	IrcProxy *proxy = ALLOCATE_OBJECT(IrcProxy);
	proxy->server = ALLOCATE_OBJECT(Socket);
	proxy->server->port = "testport";

	return proxy;
}

static void freeProxyStub(IrcProxy *proxy)
{
	free(proxy->server);
	free(proxy);
}
