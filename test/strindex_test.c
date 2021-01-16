/*
  Copyright 2011-2020 David Robillard <d@drobilla.net>

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

#include "zix/common.h"
#include "zix/strindex.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

ZIX_LOG_FUNC(1, 2)
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

int
main(void)
{
  zix_strindex_free(NULL);

  const char*  str      = "BANANA";
  const size_t str_len  = strlen(str);
  ZixStrindex* strindex = zix_strindex_new(str);

  for (size_t l = 1; l <= str_len; ++l) {
    for (size_t i = 0; i < str_len - l; ++i) {
      char*     match = NULL;
      ZixStatus ret   = zix_strindex_find(strindex, str + i, &match);
      if (ret) {
        return test_fail(
          "No match for substring at %" PRIuPTR " length %" PRIuPTR "\n", i, l);
      }

      if (strncmp(str + i, match, l)) {
        return test_fail("Bad match for substring at %" PRIuPTR
                         " length %" PRIuPTR "\n",
                         i,
                         l);
      }
    }
  }

  zix_strindex_free(strindex);
  return 0;
}
