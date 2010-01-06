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
#include "test.h"
#include "util.h"
#include "modules/markov/file_letter_source.h"
#include "modules/markov/file_word_source.h"
#include "modules/markov/entropy.h"
#include "api.h"

#define TEST_FILE "/../../src/tests/markov/darwin.txt"
#define TEST_LETTER_LEVEL 5
#define TEST_LETTER_EMIN 0.95
#define TEST_LETTER_EMAX 1.0
#define TEST_WORD_LEVEL 2
#define TEST_WORD_EMIN 1.27
#define TEST_WORD_EMAX 1.28

TEST_CASE(source);

MODULE_NAME("test_markov");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the markov module");
MODULE_VERSION(0, 1, 0);
MODULE_BCVERSION(0, 1, 0);
MODULE_DEPENDS(MODULE_DEPENDENCY("markov", 1, 1, 0));

TEST_SUITE_BEGIN(markov)
	TEST_CASE_ADD(source);
TEST_SUITE_END

TEST_CASE(source)
{
	double entropy;
	char *execpath = $$(char *, getExecutablePath)();
	GString *testfile = g_string_new(execpath);
	g_string_append(testfile, TEST_FILE);

	MarkovFileLetterSource *letter_source = $(MarkovFileLetterSource *, markov, createMarkovFileLetterSource)(testfile->str, TEST_LETTER_LEVEL);
	TEST_ASSERT(letter_source != NULL);

	entropy = $(double, markov, getMarkovEntropy)(letter_source->source);
	TEST_ASSERT(entropy >= TEST_LETTER_EMIN && entropy <= TEST_LETTER_EMAX);

	$(void, markov, freeMarkovFileLetterSource)(letter_source);

	MarkovFileWordSource *word_source = $(MarkovFileWordSource *, markov, createMarkovFileWordSource)(testfile->str, TEST_WORD_LEVEL);
	TEST_ASSERT(word_source != NULL);

	entropy = $(double, markov, getMarkovEntropy)(word_source->source);
	TEST_ASSERT(entropy >= TEST_WORD_EMIN && entropy <= TEST_WORD_EMAX);

	$(void, markov, freeMarkovFileWordSource)(word_source);

	TEST_PASS;
}
