/**
 * Copyright (c) 2009, Kalisko Project Leaders
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
 *       in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Kalisko Developers nor the names of its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdio.h>
#include "hooks.h"

HOOK_LISTENER(print_number);
HOOK_LISTENER(print_hex_number);
HOOK_LISTENER(print_strings);

int main(int argc, char **argv)
{
	initHooks();

	HOOK_ADD(number);
	HOOK_ADD(two_strings);

	HOOK_ATTACH_EX(number, print_number, "Custom string");
	HOOK_ATTACH(number, print_hex_number);
	HOOK_ATTACH(two_strings, print_strings);

	HOOK_TRIGGER(number, 1337);

	HOOK_DETACH(number, print_hex_number);

	HOOK_TRIGGER(number, -1);

	HOOK_TRIGGER(two_strings, "Hello", "World");

	freeHooks();

	return EXIT_SUCCESS;
}

HOOK_LISTENER(print_number)
{
	int num = HOOK_ARG(int);

	printf("Got a number: %d\t\t", num);
	printf("Also got a custom listener string: %s\n", (char *) custom_data);
}

HOOK_LISTENER(print_hex_number)
{
	int num = HOOK_ARG(int);

	printf("Number as hex: 0x%x\n", num);
}

HOOK_LISTENER(print_strings)
{
	char *s1 = HOOK_ARG(char *);
	char *s2 = HOOK_ARG(char *);

	printf("Two strings: %s %s\n", s1, s2);
}

