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
#include <stdlib.h>
#define API
#include "log.h"
#include "memory_alloc.h"

/**
 * Initializes the memory allocator
 */
API void initMemory()
{
	GMemVTable *table = allocateMemory(sizeof(GMemVTable));

	table->malloc = &malloc;
	table->realloc = &realloc;
	table->free = &free;
	table->calloc = NULL;
	table->try_malloc = NULL;
	table->try_realloc = NULL;

	g_mem_set_vtable(table);

	free(table);
}

/**
 * Allocate a block of memory on the heap
 *
 * @param size		the amount of memory to allocate
 * @result			a pointer to the heap block allocated
 */
API void *allocateMemory(int size)
{
	void *mem = malloc(size);

	if(mem == NULL) {
		logMessage("core", LOG_TYPE_ERROR, "Failed to allocate %d more bytes of memory", size);
		exit(EXIT_FAILURE);
	}

	return mem;
}

/**
 * Reallocate a block of memory on the heap
 *
 * @param mem		A pointer to a previously allocated memory block to be reallocated
 * @result			a pointer to the reallocated
 */
API void *reallocateMemory(void *ptr, int size)
{
	void *mem = realloc(ptr, size);

	if(mem == NULL) {
		logMessage("core", LOG_TYPE_ERROR, "Could not reallocate memory block %p to requested size of %d", ptr, size);
		exit(EXIT_FAILURE);
	}

	return mem;
}
