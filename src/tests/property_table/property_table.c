/**
 * @file
 * <h3>Copyright</h3>
 * Copyright (c) 2011, Kalisko Project Leaders
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
#include "memory_alloc.h"
#include "modules/property_table/property_table.h"

#include "api.h"

TEST_CASE(null_subject);
TEST_CASE(multiple_subjects);
TEST_CASE(free_not_existing);

MODULE_NAME("test_property_table");
MODULE_AUTHOR("The Kalisko team");
MODULE_DESCRIPTION("Test suite for the Kalisko property_table module");
MODULE_VERSION(0, 0, 1);
MODULE_BCVERSION(0, 0, 1);
MODULE_DEPENDS(MODULE_DEPENDENCY("property_table", 0, 0, 1));

TEST_SUITE_BEGIN(property_table)
	TEST_CASE_ADD(null_subject);
	TEST_CASE_ADD(multiple_subjects);
	TEST_CASE_ADD(free_not_existing);
TEST_SUITE_END

TEST_CASE(null_subject)
{
	TEST_ASSERT($(void, property_table, getPropertyTableValue)(NULL, "test") == NULL);

	$(void, property_table, setPropertyTableValue)(NULL, "test", "value");
	TEST_ASSERT(strcmp($(void, property_table, getPropertyTableValue)(NULL, "test"), "value") == 0);

	$(void, property_table, freePropertyTable)(NULL);
	TEST_ASSERT($(void, property_table, getPropertyTableValue)(NULL, "test") == NULL);

	TEST_PASS;
}

TEST_CASE(multiple_subjects)
{
	typedef struct {
		int nothing;
	} TestStruct;

	TestStruct *a = ALLOCATE_OBJECT(TestStruct);
	TestStruct *b = ALLOCATE_OBJECT(TestStruct);

	TEST_ASSERT($(void, property_table, getPropertyTableValue)(a, "test") == NULL);
	TEST_ASSERT($(void, property_table, getPropertyTableValue)(b, "test") == NULL);

	$(void, property_table, setPropertyTableValue)(a, "test", "valueA");
	TEST_ASSERT(strcmp($(void, property_table, getPropertyTableValue)(a, "test"), "valueA") == 0);
	TEST_ASSERT($(void, property_table, getPropertyTableValue)(b, "test") == NULL);

	$(void, property_table, setPropertyTableValue)(b, "test", "valueB");
	TEST_ASSERT(strcmp($(void, property_table, getPropertyTableValue)(a, "test"), "valueA") == 0);
	TEST_ASSERT(strcmp($(void, property_table, getPropertyTableValue)(b, "test"), "valueB") == 0);

	$(void, property_table, freePropertyTable)(a);
	TEST_ASSERT($(void, property_table, getPropertyTableValue)(a, "test") == NULL);
	TEST_ASSERT(strcmp($(void, property_table, getPropertyTableValue)(b, "test"), "valueB") == 0);

	$(void, property_table, freePropertyTable)(b);
	TEST_ASSERT($(void, property_table, getPropertyTableValue)(a, "test") == NULL);
	TEST_ASSERT($(void, property_table, getPropertyTableValue)(b, "test") == NULL);

	TEST_PASS;
}

TEST_CASE(free_not_existing)
{
	// just free tables that do not exists
	$(void, property_table, freePropertyTable)(NULL);
	$(void, property_table, freePropertyTable)(&"something");

	TEST_PASS;
}
