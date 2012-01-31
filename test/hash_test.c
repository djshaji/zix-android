/*
  Copyright 2011 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "zix/hash.h"

static const char* strings[] = {
	"one", "two", "three", "four", "five", "six", "seven", "eight",
	"2one", "2two", "2three", "2four", "2five", "2six", "2seven", "2eight",
	"3one", "3two", "3three", "3four", "3five", "3six", "3seven", "3eight",
	"4one", "4two", "4three", "4four", "4five", "4six", "4seven", "4eight",
	"5one", "5two", "5three", "5four", "5five", "5six", "5seven", "5eight",
	"6one", "6two", "6three", "6four", "6five", "6six", "6seven", "6eight",
	"7one", "7two", "7three", "7four", "7five", "7six", "7seven", "7eight",
	"8one", "8two", "8three", "8four", "8five", "8six", "8seven", "8eight",
	"9one", "9two", "9three", "9four", "9five", "9six", "9seven", "9eight",
	"Aone", "Atwo", "Athree", "Afour", "Afive", "Asix", "Aseven", "Aeight",
	"Bone", "Btwo", "Bthree", "Bfour", "Bfive", "Bsix", "Bseven", "Beight",
	"Cone", "Ctwo", "Cthree", "Cfour", "Cfive", "Csix", "Cseven", "Ceight",
	"Done", "Dtwo", "Dthree", "Dfour", "Dfive", "Dsix", "Dseven", "Deight",
};

static int
test_fail(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	va_end(args);
	return 1;
}

static unsigned n_checked = 0;

static void
check(const void* key, void* value, void* user_data)
{
	if (key == value) {
		++n_checked;
	} else {
		fprintf(stderr, "ERROR: %s != %s\n",
		        (const char*)key, (const char*)value);
	}
}

int
main(int argc, char** argv)
{
	ZixHash* hash = zix_hash_new(zix_string_hash, zix_string_equal);

	const size_t n_strings = sizeof(strings) / sizeof(char*);

	// Insert each string
	for (size_t i = 0; i < n_strings; ++i) {
		ZixStatus st = zix_hash_insert(hash, strings[i], (char*)strings[i]);
		if (st) {
			return test_fail("Failed to insert `%s'\n", strings[i]);
		}
	}

	//zix_hash_print_dot(hash, stdout);

	// Attempt to insert each string again
	for (size_t i = 0; i < n_strings; ++i) {
		ZixStatus st = zix_hash_insert(hash, strings[i], (char*)strings[i]);
		if (st != ZIX_STATUS_EXISTS) {
			return test_fail("Double inserted `%s'\n", strings[i]);
		}
	}

	// Search for each string
	for (size_t i = 0; i < n_strings; ++i) {
		const char* match = (const char*)zix_hash_find(hash, strings[i]);
		if (!match) {
			return test_fail("Failed to find `%s'\n", strings[i]);
		}
		if (match != strings[i]) {
			return test_fail("Bad match for `%s'\n", strings[i]);
		}
	}

	// Try some false matches
	const char* not_indexed[] = {
		"ftp://example.org/not-there-at-all",
		"http://example.org/foobar",
		"http://",
		"http://otherdomain.com"
	};
	const size_t n_not_indexed = sizeof(not_indexed) / sizeof(char*);
	for (size_t i = 0; i < n_not_indexed; ++i) {
		const char* match = (const char*)zix_hash_find(hash, not_indexed[i]);
		if (match) {
			return test_fail("Unexpectedly found `%s'\n", not_indexed[i]);
		}
	}

	// Remove strings
	for (size_t i = 0; i < n_strings; ++i) {
		// Remove string
		ZixStatus st = zix_hash_remove(hash, strings[i]);
		if (st) {
			return test_fail("Failed to remove `%s'\n", strings[i]);
		}

		// Ensure second removal fails
		st = zix_hash_remove(hash, strings[i]);
		if (st != ZIX_STATUS_NOT_FOUND) {
			return test_fail("Unexpectedly removed `%s' twice\n", strings[i]);
		}

		// Check to ensure remaining strings are still present
		for (size_t j = i + 1; j < n_strings; ++j) {
			const char* match = (const char*)zix_hash_find(hash, strings[j]);
			if (!match) {
				return test_fail("Failed to find `%s' after remove\n", strings[j]);
			}
			if (match != strings[j]) {
				return test_fail("Bad match for `%s' after remove\n", strings[j]);
			}
		}
	}

	// Insert each string again (to check non-empty desruction)
	for (size_t i = 0; i < n_strings; ++i) {
		ZixStatus st = zix_hash_insert(hash, strings[i], (char*)strings[i]);
		if (st) {
			return test_fail("Failed to insert `%s'\n", strings[i]);
		}
	}

	// Check key == value (and test zix_hash_foreach)
	zix_hash_foreach(hash, check, NULL);
	if (n_checked != n_strings) {
		return test_fail("Check failed\n");
	}

	zix_hash_free(hash);

	return 0;
}