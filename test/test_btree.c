// Copyright 2011-2021 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#undef NDEBUG

#include "zix/btree.h"

#include "failing_allocator.h"
#include "test_args.h"
#include "test_data.h"

#include "zix/allocator.h"
#include "zix/attributes.h"
#include "zix/status.h"

#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static bool expect_failure = false;

ZIX_PURE_FUNC
static int
int_cmp(const void* a, const void* b, const void* ZIX_UNUSED(user_data))
{
  const uintptr_t ia = (uintptr_t)a;
  const uintptr_t ib = (uintptr_t)b;

  assert(ia != 0U); // No wildcards
  assert(ib != 0U); // No wildcards

  return ia < ib ? -1 : ia > ib ? 1 : 0;
}

static uintptr_t
ith_elem(const unsigned test_num, const size_t n_elems, const size_t i)
{
  switch (test_num % 3) {
  case 0:
    return i + 1; // Increasing
  case 1:
    return n_elems - i; // Decreasing
  default:
    return 1U + unique_rand(i); // Pseudo-random
  }
}

typedef struct {
  unsigned test_num;
  size_t   n_elems;
} TestContext;

static uintptr_t
wildcard_cut(unsigned test_num, size_t n_elems)
{
  return ith_elem(test_num, n_elems, n_elems / 3);
}

/// Wildcard comparator where 0 matches anything >= wildcard_cut(n_elems)
static int
wildcard_cmp(const void* a, const void* b, const void* user_data)
{
  const TestContext* ctx      = (const TestContext*)user_data;
  const size_t       n_elems  = ctx->n_elems;
  const unsigned     test_num = ctx->test_num;
  const uintptr_t    ia       = (uintptr_t)a;
  const uintptr_t    ib       = (uintptr_t)b;

  if (ia == 0) {
    if (ib >= wildcard_cut(test_num, n_elems)) {
      return 0; // Wildcard match
    }

    return 1; // Wildcard a > b
  }

  if (ib == 0) {
    if (ia >= wildcard_cut(test_num, n_elems)) {
      return 0; // Wildcard match
    }

    return -1; // Wildcard b > a
  }

  return int_cmp(a, b, user_data);
}

ZIX_LOG_FUNC(2, 3)
static int
test_fail(ZixBTree* t, const char* fmt, ...)
{
  zix_btree_free(t, NULL, NULL);
  if (expect_failure) {
    return EXIT_SUCCESS;
  }

  va_list args; // NOLINT(cppcoreguidelines-init-variables)
  va_start(args, fmt);
  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, args);
  va_end(args);
  return EXIT_FAILURE;
}

static const size_t n_clear_insertions = 1024U;

static void
destroy(void* const ptr, const void* const user_data)
{
  (void)user_data;
  assert(ptr);
  assert((uintptr_t)ptr <= n_clear_insertions);
}

static void
no_destroy(void* const ptr, const void* const user_data)
{
  (void)ptr;
  (void)user_data;
  assert(!ptr);
}

static void
test_clear(void)
{
  ZixBTree* t = zix_btree_new(NULL, int_cmp, NULL);

  for (uintptr_t r = 0U; r < n_clear_insertions; ++r) {
    assert(!zix_btree_insert(t, (void*)(r + 1U)));
  }

  zix_btree_clear(t, destroy, NULL);
  assert(zix_btree_size(t) == 0);

  zix_btree_free(t, no_destroy, NULL);
}

static void
test_free(void)
{
  ZixBTree* t = zix_btree_new(NULL, int_cmp, NULL);

  for (uintptr_t r = 0U; r < n_clear_insertions; ++r) {
    assert(!zix_btree_insert(t, (void*)(r + 1U)));
  }

  assert(zix_btree_size(t) == n_clear_insertions);

  zix_btree_free(t, destroy, NULL);
}

static void
test_iter_comparison(void)
{
  static const size_t n_elems = 4096U;

  ZixBTree* const t = zix_btree_new(NULL, int_cmp, NULL);

  // Store increasing numbers from 1 (jammed into the pointers themselves)
  for (uintptr_t r = 1U; r < n_elems; ++r) {
    assert(!zix_btree_insert(t, (void*)r));
  }

  // Check that begin and end work sensibly
  const ZixBTreeIter begin = zix_btree_begin(t);
  const ZixBTreeIter end   = zix_btree_end(t);
  assert(!zix_btree_iter_is_end(begin));
  assert(zix_btree_iter_is_end(end));
  assert(!zix_btree_iter_equals(begin, end));
  assert(!zix_btree_iter_equals(end, begin));

  // Make another begin iterator
  ZixBTreeIter j = zix_btree_begin(t);
  assert(zix_btree_iter_equals(begin, j));

  // Advance it and check that they are no longer equal
  for (size_t r = 1U; r < n_elems - 1U; ++r) {
    j = zix_btree_iter_next(j);
    assert(!zix_btree_iter_is_end(j));
    assert(!zix_btree_iter_equals(begin, j));
    assert(!zix_btree_iter_equals(end, j));
    assert(!zix_btree_iter_equals(j, end));
  }

  // Advance it to the end
  zix_btree_iter_increment(&j);
  assert(zix_btree_iter_is_end(j));
  assert(!zix_btree_iter_equals(begin, j));
  assert(zix_btree_iter_equals(end, j));
  assert(zix_btree_iter_equals(j, end));

  zix_btree_free(t, NULL, NULL);
}

static void
test_insert_split_value(void)
{
  static const size_t    n_insertions = 767U; // Number of insertions to split
  static const uintptr_t split_value  = 512U; // Value that will be pulled up

  ZixBTree* const t = zix_btree_new(NULL, int_cmp, NULL);

  // Insert right up until it would cause a split
  for (uintptr_t r = 1U; r < n_insertions; ++r) {
    assert(!zix_btree_insert(t, (void*)r));
  }

  // Insert the element that will be chosen as the split pivot
  assert(zix_btree_insert(t, (void*)split_value) == ZIX_STATUS_EXISTS);

  zix_btree_free(t, NULL, NULL);
}

static void
test_remove_cases(void)
{
  /* Insert and remove in several "phases" with different strides that are not
     even multiples.  This spreads the load around to hit as many cases as
     possible. */

  static const uintptr_t s1           = 3U;
  static const uintptr_t s2           = 511U;
  const size_t           n_insertions = s1 * s2 * 450U;

  ZixBTree* const t = zix_btree_new(NULL, int_cmp, NULL);

  // Insert in s1-sized chunks
  for (uintptr_t phase = 0U; phase < s1; ++phase) {
    for (uintptr_t r = 0U; r < n_insertions / s1; ++r) {
      const uintptr_t value = (s1 * r) + phase + 1U;

      assert(!zix_btree_insert(t, (void*)value));
    }
  }

  // Remove in s2-sized chunks
  ZixBTreeIter next = zix_btree_end(t);
  for (uintptr_t phase = 0U; phase < s2; ++phase) {
    for (uintptr_t r = 0U; r < n_insertions / s2; ++r) {
      const uintptr_t value = (s2 * r) + phase + 1U;
      void*           out   = NULL;

      assert(!zix_btree_remove(t, (void*)value, &out, &next));
      assert((uintptr_t)out == value);
    }
  }

  assert(!zix_btree_size(t));
  zix_btree_free(t, NULL, NULL);
}

static int
stress(ZixAllocator* const allocator,
       const unsigned      test_num,
       const size_t        n_elems)
{
  if (n_elems == 0) {
    return 0;
  }

  uintptr_t r  = 0;
  ZixBTree* t  = zix_btree_new(allocator, int_cmp, NULL);
  ZixStatus st = ZIX_STATUS_SUCCESS;

  if (!t) {
    return test_fail(t, "Failed to allocate tree\n");
  }

  // Ensure begin iterator is end on empty tree
  ZixBTreeIter ti  = zix_btree_begin(t);
  ZixBTreeIter end = zix_btree_end(t);

  if (!zix_btree_iter_is_end(ti)) {
    return test_fail(t, "Begin iterator on empty tree is not end\n");
  }

  if (!zix_btree_iter_equals(ti, end)) {
    return test_fail(t, "Begin and end of empty tree are not equal\n");
  }

  // Insert n_elems elements
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (!zix_btree_find(t, (void*)r, &ti)) {
      return test_fail(t, "%" PRIuPTR " already in tree\n", (uintptr_t)r);
    }

    if ((st = zix_btree_insert(t, (void*)r))) {
      return test_fail(
        t, "Insert %" PRIuPTR " failed (%s)\n", (uintptr_t)r, zix_strerror(st));
    }
  }

  // Ensure tree size is correct
  if (zix_btree_size(t) != n_elems) {
    return test_fail(t,
                     "Tree size %" PRIuPTR " != %" PRIuPTR "\n",
                     zix_btree_size(t),
                     n_elems);
  }

  // Ensure begin no longer equals end
  ti  = zix_btree_begin(t);
  end = zix_btree_end(t);
  if (zix_btree_iter_equals(ti, end)) {
    return test_fail(t, "Begin and end of non-empty tree are equal\n");
  }

  // Search for all elements
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (zix_btree_find(t, (void*)r, &ti)) {
      return test_fail(
        t, "Find %" PRIuPTR " @ %" PRIuPTR " failed\n", (uintptr_t)r, i);
    }

    if ((uintptr_t)zix_btree_get(ti) != r) {
      return test_fail(t,
                       "Search data corrupt (%" PRIuPTR " != %" PRIuPTR ")\n",
                       (uintptr_t)zix_btree_get(ti),
                       r);
    }
  }

  // Find the lower bound of all elements and ensure it's exact
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (zix_btree_lower_bound(t, int_cmp, NULL, (void*)r, &ti)) {
      return test_fail(
        t, "Lower bound %" PRIuPTR " @ %" PRIuPTR " failed\n", (uintptr_t)r, i);
    }

    if (zix_btree_iter_is_end(ti)) {
      return test_fail(t,
                       "Lower bound %" PRIuPTR " @ %" PRIuPTR " hit end\n",
                       (uintptr_t)r,
                       i);
    }

    if ((uintptr_t)zix_btree_get(ti) != r) {
      return test_fail(t,
                       "Lower bound corrupt (%" PRIuPTR " != %" PRIuPTR "\n",
                       (uintptr_t)zix_btree_get(ti),
                       r);
    }
  }

  // Search for elements that don't exist
  for (size_t i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems * 3, n_elems + i);
    if (!zix_btree_find(t, (void*)r, &ti)) {
      return test_fail(t, "Unexpectedly found %" PRIuPTR "\n", (uintptr_t)r);
    }
  }

  // Iterate over all elements
  size_t    i    = 0;
  uintptr_t last = 0;
  for (ti = zix_btree_begin(t); !zix_btree_iter_is_end(ti);
       zix_btree_iter_increment(&ti), ++i) {
    const uintptr_t iter_data = (uintptr_t)zix_btree_get(ti);
    if (iter_data < last) {
      return test_fail(t,
                       "Iter @ %" PRIuPTR " corrupt (%" PRIuPTR " < %" PRIuPTR
                       ")\n",
                       i,
                       iter_data,
                       last);
    }
    last = iter_data;
  }

  if (i != n_elems) {
    return test_fail(t,
                     "Iteration stopped at %" PRIuPTR "/%" PRIuPTR
                     " elements\n",
                     i,
                     n_elems);
  }

  // Insert n_elems elements again, ensuring duplicates fail
  for (i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if (!zix_btree_insert(t, (void*)r)) {
      return test_fail(t, "Duplicate insert succeeded\n");
    }
  }

  // Search for the middle element then iterate from there
  r = ith_elem(test_num, n_elems, n_elems / 2);
  if (zix_btree_find(t, (void*)r, &ti)) {
    return test_fail(t, "Find %" PRIuPTR " failed\n", (uintptr_t)r);
  }
  last = (uintptr_t)zix_btree_get(ti);
  zix_btree_iter_increment(&ti);
  for (i = 1; !zix_btree_iter_is_end(ti); zix_btree_iter_increment(&ti), ++i) {
    if ((uintptr_t)zix_btree_get(ti) == last) {
      return test_fail(
        t, "Duplicate element @ %" PRIuPTR " %" PRIuPTR "\n", i, last);
    }
    last = (uintptr_t)zix_btree_get(ti);
  }

  // Delete all elements
  ZixBTreeIter next = zix_btree_end_iter;
  for (size_t e = 0; e < n_elems; e++) {
    r = ith_elem(test_num, n_elems, e);

    uintptr_t removed = 0;
    if (zix_btree_remove(t, (void*)r, (void**)&removed, &next)) {
      return test_fail(t, "Error removing item %" PRIuPTR "\n", (uintptr_t)r);
    }

    if (removed != r) {
      return test_fail(t,
                       "Removed wrong item %" PRIuPTR " != %" PRIuPTR "\n",
                       removed,
                       (uintptr_t)r);
    }

    if (test_num == 0) {
      const uintptr_t next_value = ith_elem(test_num, n_elems, e + 1);
      if (!((zix_btree_iter_is_end(next) && e == n_elems - 1) ||
            (uintptr_t)zix_btree_get(next) == next_value)) {
        return test_fail(t,
                         "Delete all next iterator %" PRIuPTR " != %" PRIuPTR
                         "\n",
                         (uintptr_t)zix_btree_get(next),
                         next_value);
      }
    }
  }

  // Ensure the tree is empty
  if (zix_btree_size(t) != 0) {
    return test_fail(t, "Tree size %" PRIuPTR " != 0\n", zix_btree_size(t));
  }

  // Insert n_elems elements again (to test non-empty destruction)
  for (size_t e = 0; e < n_elems; ++e) {
    r = ith_elem(test_num, n_elems, e);
    if (zix_btree_insert(t, (void*)r)) {
      return test_fail(t, "Post-deletion insert failed\n");
    }
  }

  // Delete elements that don't exist
  for (size_t e = 0; e < n_elems; e++) {
    r                 = ith_elem(test_num, n_elems * 3, n_elems + e);
    uintptr_t removed = 0;
    if (!zix_btree_remove(t, (void*)r, (void**)&removed, &next)) {
      return test_fail(
        t, "Non-existant deletion of %" PRIuPTR " succeeded\n", (uintptr_t)r);
    }
  }

  // Ensure tree size is still correct
  if (zix_btree_size(t) != n_elems) {
    return test_fail(t,
                     "Tree size %" PRIuPTR " != %" PRIuPTR "\n",
                     zix_btree_size(t),
                     n_elems);
  }

  // Delete some elements towards the end
  for (size_t e = 0; e < n_elems / 4; e++) {
    r = ith_elem(test_num, n_elems, n_elems - (n_elems / 4) + e);
    uintptr_t removed = 0;
    if (zix_btree_remove(t, (void*)r, (void**)&removed, &next)) {
      return test_fail(t, "Deletion of %" PRIuPTR " failed\n", (uintptr_t)r);
    }

    if (removed != r) {
      return test_fail(t,
                       "Removed wrong item %" PRIuPTR " != %" PRIuPTR "\n",
                       removed,
                       (uintptr_t)r);
    }

    if (test_num == 0) {
      const uintptr_t next_value = ith_elem(test_num, n_elems, e + 1);
      if (!zix_btree_iter_is_end(next) &&
          (uintptr_t)zix_btree_get(next) == next_value) {
        return test_fail(t,
                         "Next iterator %" PRIuPTR " != %" PRIuPTR "\n",
                         (uintptr_t)zix_btree_get(next),
                         next_value);
      }
    }
  }

  // Check tree size
  if (zix_btree_size(t) != n_elems - (n_elems / 4)) {
    return test_fail(t,
                     "Tree size %" PRIuPTR " != %" PRIuPTR "\n",
                     zix_btree_size(t),
                     n_elems);
  }

  // Delete some elements in a random order
  for (size_t e = 0; e < zix_btree_size(t) / 2; e++) {
    r = ith_elem(test_num, n_elems, unique_rand(e) % n_elems);

    uintptr_t removed = 0;
    ZixStatus rst     = zix_btree_remove(t, (void*)r, (void**)&removed, &next);
    if (rst != ZIX_STATUS_SUCCESS && rst != ZIX_STATUS_NOT_FOUND) {
      return test_fail(t, "Error deleting %" PRIuPTR "\n", (uintptr_t)r);
    }
  }

  // Delete all remaining elements via next iterator
  next                 = zix_btree_begin(t);
  uintptr_t last_value = 0;
  while (!zix_btree_iter_is_end(next)) {
    const uintptr_t value   = (uintptr_t)zix_btree_get(next);
    uintptr_t       removed = 0;
    if (zix_btree_remove(t, zix_btree_get(next), (void**)&removed, &next)) {
      return test_fail(
        t, "Error removing next item %" PRIuPTR "\n", (uintptr_t)r);
    }

    if (removed != value) {
      return test_fail(t,
                       "Removed wrong next item %" PRIuPTR " != %" PRIuPTR "\n",
                       removed,
                       (uintptr_t)value);
    }

    if (removed < last_value) {
      return test_fail(t,
                       "Removed unordered next item %" PRIuPTR " < %" PRIuPTR
                       "\n",
                       removed,
                       (uintptr_t)value);
    }

    last_value = removed;
  }

  assert(zix_btree_size(t) == 0);
  zix_btree_free(t, NULL, NULL);

  // Test lower_bound with wildcard comparator

  TestContext ctx = {test_num, n_elems};
  if (!(t = zix_btree_new(NULL, wildcard_cmp, &ctx))) {
    return test_fail(t, "Failed to allocate tree\n");
  }

  // Insert n_elems elements
  for (i = 0; i < n_elems; ++i) {
    r = ith_elem(test_num, n_elems, i);
    if ((st = zix_btree_insert(t, (void*)r))) {
      return test_fail(
        t, "Insert %" PRIuPTR " failed (%s)\n", (uintptr_t)r, zix_strerror(st));
    }
  }

  // Find lower bound of wildcard
  const uintptr_t wildcard = 0;
  if (zix_btree_lower_bound(t, wildcard_cmp, &ctx, (void*)wildcard, &ti)) {
    return test_fail(t, "Lower bound failed\n");
  }

  if (zix_btree_iter_is_end(ti)) {
    return test_fail(t, "Lower bound reached end\n");
  }

  // Check value
  const uintptr_t iter_data = (uintptr_t)zix_btree_get(ti);
  const uintptr_t cut       = wildcard_cut(test_num, n_elems);
  if (iter_data != cut) {
    return test_fail(
      t, "Lower bound %" PRIuPTR " != %" PRIuPTR "\n", iter_data, cut);
  }

  if (wildcard_cmp((void*)wildcard, (void*)iter_data, &ctx)) {
    return test_fail(
      t, "Wildcard lower bound %" PRIuPTR " != %" PRIuPTR "\n", iter_data, cut);
  }

  // Find lower bound of value past end
  const uintptr_t max = (uintptr_t)-1;
  if (zix_btree_lower_bound(t, wildcard_cmp, &ctx, (void*)max, &ti)) {
    return test_fail(t, "Lower bound failed\n");
  }

  if (!zix_btree_iter_is_end(ti)) {
    return test_fail(t, "Lower bound of maximum value is not end\n");
  }

  zix_btree_free(t, NULL, NULL);

  return EXIT_SUCCESS;
}

static void
test_failed_alloc(void)
{
  ZixFailingAllocator allocator = zix_failing_allocator();

  // Successfully stress test the tree to count the number of allocations
  assert(!stress(&allocator.base, 0, 4096));

  // Test that each allocation failing is handled gracefully
  const size_t n_new_allocs = allocator.n_allocations;
  for (size_t i = 0U; i < n_new_allocs; ++i) {
    allocator.n_remaining = i;
    assert(stress(&allocator.base, 0, 4096));
  }
}

int
main(int argc, char** argv)
{
  if (argc > 2) {
    fprintf(stderr, "Usage: %s [N_ELEMS]\n", argv[0]);
    return EXIT_FAILURE;
  }

  test_clear();
  test_free();
  test_iter_comparison();
  test_insert_split_value();
  test_remove_cases();
  test_failed_alloc();

  const unsigned n_tests = 2U;
  const size_t   n_elems =
    (argc > 1) ? zix_test_size_arg(argv[1], 4U, 1U << 20U) : (1U << 16U);

  printf("Running %u tests with %zu elements", n_tests, n_elems);
  for (unsigned i = 0; i < n_tests; ++i) {
    printf(".");
    fflush(stdout);
    if (stress(NULL, i, n_elems)) {
      return EXIT_FAILURE;
    }
  }
  printf("\n");

  return EXIT_SUCCESS;
}
