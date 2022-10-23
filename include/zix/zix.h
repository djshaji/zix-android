// Copyright 2016-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_ZIX_H
#define ZIX_ZIX_H

// IWYU pragma: begin_exports

/**
   @defgroup zix Zix C API
   @{
*/

/**
   @defgroup zix_utilities Utilities
   @{
*/

#include "zix/attributes.h"
#include "zix/digest.h"
#include "zix/function_types.h"
#include "zix/status.h"
#include "zix/string_view.h"

/**
   @}
   @defgroup zix_allocation Allocation
   @{
*/

#include "zix/allocator.h"
#include "zix/bump_allocator.h"

/**
   @}
   @defgroup zix_data_structures Data Structures
   @{
*/

#include "zix/bitset.h"
#include "zix/btree.h"
#include "zix/hash.h"
#include "zix/ring.h"
#include "zix/tree.h"

/**
   @}
   @defgroup zix_threading Threading
   @{
*/

#include "zix/sem.h"
#include "zix/thread.h"

/**
   @}
   @}
*/

// IWYU pragma: end_exports

#endif /* ZIX_ZIX_H */
