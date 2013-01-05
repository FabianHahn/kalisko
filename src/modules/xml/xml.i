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

#ifndef XML_XML_H
#define XML_XML_H

#include <libxml/tree.h>
#include <glib.h>


/**
 * Parses an XML string
 *
 * @param xml		the XML string to parse
 * @result			the resulting XML document pointer of NULL on failure
 */
API xmlDocPtr parseXmlString(const char *xml);

/**
 * Evaluates an XPath expression on a parsed XML document tree
 *
 * @param document			the document in which to evaluate the XPath expression
 * @param xpath				the XPath expression to evaluate
 * @result					a list containing all the result strings or NULL on failure
 */
API GQueue *evaluateXPathExpression(xmlDocPtr document, const char *xpath);

/**
 * Evaluates an XPath expression on a parsed XML document tree, but returns only the first result
 *
 * @param document			the document in which to evaluate the XPath expression
 * @param xpath				the XPath expression to evaluate
 * @result					the first matching result or NULL on failure
 */
API GString *evaluateXPathExpressionFirst(xmlDocPtr document, const char *xpath);

/**
 * Frees a results list as returned by evaluateXPathExpression
 *
 * @param results			the results list to free
 */
API void freeXPathExpressionResults(GQueue *results);

#endif
