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

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <glib.h>

#include "dll.h"

#define API
#include "xml.h"

MODULE_NAME("xml");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("libxml2 library access");
MODULE_VERSION(0, 1, 1);
MODULE_BCVERSION(0, 1, 0);
MODULE_NODEPS;

static void libxmlErrorHandler(void *context, const char *message, ...);

MODULE_INIT
{
	xmlInitParser();
	xmlSetGenericErrorFunc(NULL, &libxmlErrorHandler);

	return true;
}

MODULE_FINALIZE
{
	xmlCleanupParser();
}

/**
 * Parses an XML string
 *
 * @param xml		the XML string to parse
 * @result			the resulting XML document pointer of NULL on failure
 */
API xmlDocPtr parseXmlString(const char *xml)
{
	xmlDocPtr document;
	if((document = xmlParseMemory(xml, strlen(xml))) == NULL) {
		LOG_ERROR("Failed to parse XML string");
		return NULL;
	}

	return document;
}

/**
 * Evaluates an XPath expression on a parsed XML document tree
 *
 * @param document			the document in which to evaluate the XPath expression
 * @param xpath				the XPath expression to evaluate
 * @result					a list containing all the result strings or NULL on failure
 */
API GQueue *evaluateXPathExpression(xmlDocPtr document, const char *xpath)
{
	xmlXPathContextPtr context;
	if((context = xmlXPathNewContext(document)) == NULL) {
		LOG_ERROR("Failed to create XPath context for document");
		return NULL;
	}

	xmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar *) xpath, context);
	xmlXPathFreeContext(context);

	if(result == NULL) {
		LOG_ERROR("Failed to execute XPath expression: %s", xpath);
		return NULL;
	}

	xmlNodeSetPtr nodeset = result->nodesetval;
	GQueue *results = g_queue_new();

	if(xmlXPathNodeSetIsEmpty(nodeset)){
		for(int i = 0; i < nodeset->nodeNr; i++) {
			xmlChar *resultString = xmlNodeListGetString(document, nodeset->nodeTab[i]->xmlChildrenNode, 1);
			g_queue_push_tail(results, g_string_new((char *) resultString));
			xmlFree(resultString);
		}
	}

	xmlXPathFreeObject(result);

	return results;
}

static void libxmlErrorHandler(void *context, const char *message, ...)
{
	va_list va;
	va_start(va, message);

	GString *error = g_string_new("");
	g_string_append_vprintf(error, message, va);
	LOG_ERROR("libxml2 error: %s", error->str);
	g_string_free(error, true);
}
