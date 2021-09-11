// Copyright 2016-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_COMMON_H
#define ZIX_COMMON_H

#include <stdbool.h>

/**
   @addtogroup zix
   @{
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  ZIX_STATUS_SUCCESS,
  ZIX_STATUS_ERROR,
  ZIX_STATUS_NO_MEM,
  ZIX_STATUS_NOT_FOUND,
  ZIX_STATUS_EXISTS,
  ZIX_STATUS_BAD_ARG,
  ZIX_STATUS_BAD_PERMS,
  ZIX_STATUS_REACHED_END
} ZixStatus;

static inline const char*
zix_strerror(const ZixStatus status)
{
  switch (status) {
  case ZIX_STATUS_SUCCESS:
    return "Success";
  case ZIX_STATUS_ERROR:
    return "Unknown error";
  case ZIX_STATUS_NO_MEM:
    return "Out of memory";
  case ZIX_STATUS_NOT_FOUND:
    return "Not found";
  case ZIX_STATUS_EXISTS:
    return "Exists";
  case ZIX_STATUS_BAD_ARG:
    return "Bad argument";
  case ZIX_STATUS_BAD_PERMS:
    return "Bad permissions";
  case ZIX_STATUS_REACHED_END:
    return "Reached end";
  }
  return "Unknown error";
}

/// Function for comparing two elements
typedef int (*ZixComparator)(const void* a,
                             const void* b,
                             const void* user_data);

/// Function for testing equality of two elements
typedef bool (*ZixEqualFunc)(const void* a, const void* b);

/// Function to destroy an element
typedef void (*ZixDestroyFunc)(void* ptr, const void* user_data);

/**
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIX_COMMON_H */
