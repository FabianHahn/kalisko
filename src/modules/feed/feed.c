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
#include "dll.h"
#include "modules/curl/curl.h"
#include "modules/xml/xml.h"
#define API

MODULE_NAME("feed");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Module to track XML feeds");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("xml", 0, 1, 2), MODULE_DEPENDENCY("curl", 0, 1, 1));

MODULE_INIT
{
	GString *xml = curlRequestUrl("http://generations.fr/winradio/prog.xml");

	xmlDocPtr document = parseXmlString(xml->str);
	GString *title = evaluateXPathExpressionFirst(document, "/prog/morceau[@id='1']/chanson");
	GString *artist = evaluateXPathExpressionFirst(document, "/prog/morceau[@id='1']/chanteur");
	xmlFreeDoc(document);

	if(title != NULL) {
		LOG_INFO("Currently title: %s", title->str);
		g_string_free(title, true);
	}

	if(artist != NULL) {
		LOG_INFO("Currently artist: %s", artist->str);
		g_string_free(artist, true);
	}

	return true;
}

MODULE_FINALIZE
{

}
