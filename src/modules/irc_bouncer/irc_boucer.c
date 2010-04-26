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

#include "dll.h"
#include "hooks.h"
#include "log.h"
#include "types.h"
#include "modules/irc/irc.h"
#include "modules/irc_proxy/irc_proxy.h"
#include "modules/config/config.h"
#include "modules/store/store.h"
#include "modules/store/path.h"
#include "api.h"


MODULE_NAME("irc_boucer");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("A simple IRC bouncer using an IRC connection to a single IRC server on a listening port");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("irc", 0, 2, 7), MODULE_DEPENDENCY("irc_proxy", 0, 1, 1), MODULE_DEPENDENCY("config", 0, 2, 3), MODULE_DEPENDENCY("store", 0, 6, 0));

static IrcConnection *irc;
static IrcProxy *proxy;

MODULE_INIT
{
	Store *config = $(Store *, config, getConfigPathValue)("irc");

	if(config == NULL) {
		return false;
	}

	if((irc = $(IrcConnection *, irc, createIrcConnectionByStore)(config)) == NULL) {
		LOG_ERROR("Failed to establich remote IRC connection, aborting IRC bouncer");
		return false;
	}

	Store *bouncer = $(Store *, config, getConfigPathValue)("irc/bouncer");

	if(bouncer == NULL) {
		return false;
	}

	Store *param;
	char *port;
	char *password;

	if((param = $(Store *, config, getStorePath)(bouncer, "port")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'port', aborting IRC bouncer");
		return false;
	}

	port = param->content.string;

	if((param = $(Store *, config, getStorePath)(bouncer, "password")) == NULL || param->type != STORE_STRING) {
		LOG_ERROR("Could not find required params value 'password', aborting IRC bouncer");
		return false;
	}

	password = param->content.string;

	proxy = $(IrcProxy *, irc_proxy, createIrcProxy)(irc, port, password);

	return true;
}

MODULE_FINALIZE
{

}
