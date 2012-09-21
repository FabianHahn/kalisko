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

#include <glib.h>
#include <stdlib.h>
#include "dll.h"

#define API
#include "modules/httpserver/httpserver.h"

#define PORT "1337"
#define MATCH_EVERYTHING ".*"

MODULE_NAME("httpserver_demo");
MODULE_AUTHOR("Dino Wernli");
MODULE_DESCRIPTION("This module provides a basic http server which demonstrates how to use the http server library.");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("httpserver", 0, 0, 1));

static HttpServer *server;

static bool demoHandler(HttpRequest *request, HttpResponse *response)
{
    response->content = "Hi. I am the demo handler.\n\nAnd btw, hello world!";
    return true;
}

MODULE_INIT
{
  server = createHttpServer(PORT);
  registerRequestHandler(server, MATCH_EVERYTHING, &demoHandler);
  if (!startHttpServer(server)) {
    LOG_ERROR("Failed to start HTTP server");
    return false;
  }
    return true;
}

MODULE_FINALIZE
{
  freeHttpServer(server);
}
