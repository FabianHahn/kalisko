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

#include <stdio.h>
#include <glib.h>

#include "dll.h"
#include "log.h"
#include "modules/config_standard/util.h"
#include "api.h"
#include "irc.h"

MODULE_NAME("irc");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("This module connects to an IRC server and does basic communication to keep the connection alive");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("config_standard", 0, 1, 1), MODULE_DEPENDENCY("socket", 0, 2, 0));

MODULE_INIT
{
	ConfigNodeValue *config;
	char *server;
	char *port;
	char *user;
	char *nick;

	if((config = $(ConfigNodeValue *, config_standard, getStandardConfigPathValue)("irc/server")) == NULL || config->type != CONFIG_STRING) {
		LOG_ERROR("Could not find required config value 'irc/server', aborting");
		return false;
	}

	server = config->content.string;

	if((config = $(ConfigNodeValue *, config_standard, getStandardConfigPathValue)("irc/port")) == NULL || config->type != CONFIG_STRING) {
		LOG_ERROR("Could not find required config value 'irc/port', aborting");
		return false;
	}

	port = config->content.string;

	if((config = $(ConfigNodeValue *, config_standard, getStandardConfigPathValue)("irc/user")) == NULL || config->type != CONFIG_STRING) {
		LOG_ERROR("Could not find required config value 'irc/user', aborting");
		return false;
	}

	user = config->content.string;

	if((config = $(ConfigNodeValue *, config_standard, getStandardConfigPathValue)("irc/nick")) == NULL || config->type != CONFIG_STRING) {
		LOG_ERROR("Could not find required config value 'irc/nick', aborting");
		return false;
	}

	nick = config->content.string;

	return true;
}

MODULE_FINALIZE
{

}
