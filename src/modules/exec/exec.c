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
#include "log.h"
#include "types.h"
#include "modules/socket/socket.h"
#include "modules/socket/poll.h"
#define API
#include "exec.h"

MODULE_NAME("exec");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("The exec module offers a simple interface to execute shell commands and return their output");
MODULE_VERSION(0, 1, 2);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("socket", 0, 6, 3));

MODULE_INIT
{
	return true;
}

MODULE_FINALIZE
{

}

/**
 * Executes a shell command and returns its output. Note that this function blocks until the command finished its execution.
 * For now, this command just splits arguments by spaces, that is, no arguments containing spaces are possible
 *
 * @param command			the command to execute
 * @result					the output of the command, or NULL if failed
 */
API GString *executeShellCommand(char *command)
{
	char **args = g_strsplit(command, " ", 0);
	GString *ret = executeShellCommandArgs(args);
	g_strfreev(args);
	return ret;
}

/**
 * Executes a shell command by a list of arguments and returns its output. Note that this function blocks until the command finished its execution.
 *
 * @param command			the command args to execute
 * @result					the output of the command, or NULL if failed
 */
API GString *executeShellCommandArgs(char **args)
{
	Socket *socket = $(Socket *, socket, createShellSocket)(args);

	if(!$(Socket *, socket, connectSocket)(socket)) {
		$(void, socket, freeSocket)(socket);
		return NULL;
	}

	GString *ret = g_string_new("");
	char buffer[SOCKET_POLL_BUFSIZE];

	while(socket->connected) {
		int size = $(int, socket, socketReadRaw)(socket, buffer, SOCKET_POLL_BUFSIZE);

		if(size > 0) {
			g_string_append_len(ret, buffer, size);
		}
	}

	freeSocket(socket);

	return ret;
}
