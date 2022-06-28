// Copyright 2011-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef ZIX_TREE_H
#define ZIX_TREE_H

#include "zix/allocator.h"
#include "zix/attributes.h"
#include "zix/common.h"

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   @addtogroup zix
   @{
   @name Tree
   @{
*/

/// A balanced binary search tree
typedef struct ZixTreeImpl ZixTree;

/// An iterator over a @ref ZixTree
typedef struct ZixTreeNodeImpl ZixTreeIter;

/// Create a new (empty) tree
ZIX_API
ZixTree* ZIX_ALLOCATED
zix_tree_new(ZixAllocator* ZIX_NULLABLE  allocator,
             bool                        allow_duplicates,
             ZixComparator ZIX_NONNULL   cmp,
             void* ZIX_NULLABLE          cmp_data,
             ZixDestroyFunc ZIX_NULLABLE destroy,
             const void* ZIX_NULLABLE    destroy_user_data);

/// Free `t`
ZIX_API
void
zix_tree_free(ZixTree* ZIX_NULLABLE t);

/// Return the number of elements in `t`
ZIX_PURE_API
size_t
zix_tree_size(const ZixTree* ZIX_NONNULL t);

/// Insert the element `e` into `t` and point `ti` at the new element
ZIX_API
ZixStatus
zix_tree_insert(ZixTree* ZIX_NONNULL                    t,
                void* ZIX_NULLABLE                      e,
                ZixTreeIter* ZIX_NULLABLE* ZIX_NULLABLE ti);

/// Remove the item pointed at by `ti` from `t`
ZIX_API
ZixStatus
zix_tree_remove(ZixTree* ZIX_NONNULL t, ZixTreeIter* ZIX_NONNULL ti);

/**
   Set `ti` to an element equal to `e` in `t`.

   If no such item exists, `ti` is set to NULL.
*/
ZIX_API
ZixStatus
zix_tree_find(const ZixTree* ZIX_NONNULL             t,
              const void* ZIX_NULLABLE               e,
              ZixTreeIter* ZIX_NULLABLE* ZIX_NONNULL ti);

/// Return the data associated with the given tree item
ZIX_PURE_API
void* ZIX_NULLABLE
zix_tree_get(const ZixTreeIter* ZIX_NULLABLE ti);

/// Return an iterator to the first (smallest) element in `t`
ZIX_PURE_API
ZixTreeIter* ZIX_NULLABLE
zix_tree_begin(ZixTree* ZIX_NONNULL t);

/// Return an iterator the the element one past the last element in `t`
ZIX_CONST_API
ZixTreeIter* ZIX_NULLABLE
zix_tree_end(ZixTree* ZIX_NONNULL t);

/// Return true iff `i` is an iterator to the end of its tree
ZIX_CONST_API
bool
zix_tree_iter_is_end(const ZixTreeIter* ZIX_NULLABLE i);

/// Return an iterator to the last (largest) element in `t`
ZIX_PURE_API
ZixTreeIter* ZIX_NULLABLE
zix_tree_rbegin(ZixTree* ZIX_NONNULL t);

/// Return an iterator the the element one before the first element in `t`
ZIX_CONST_API
ZixTreeIter* ZIX_NULLABLE
zix_tree_rend(ZixTree* ZIX_NONNULL t);

/// Return true iff `i` is an iterator to the reverse end of its tree
ZIX_CONST_API
bool
zix_tree_iter_is_rend(const ZixTreeIter* ZIX_NULLABLE i);

/// Return an iterator that points to the element one past `i`
ZIX_PURE_API
ZixTreeIter* ZIX_NULLABLE
zix_tree_iter_next(ZixTreeIter* ZIX_NULLABLE i);

/// Return an iterator that points to the element one before `i`
ZIX_PURE_API
ZixTreeIter* ZIX_NULLABLE
zix_tree_iter_prev(ZixTreeIter* ZIX_NULLABLE i);

/**
   @}
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIX_TREE_H */
