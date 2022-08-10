// Copyright 2011-2022 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "zix/ring.h"

#include "zix_config.h"

#include <stdlib.h>
#include <string.h>

#if USE_MLOCK
#  include <sys/mman.h>
#  define ZIX_MLOCK(ptr, size) mlock((ptr), (size))
#elif defined(_WIN32)
#  include <windows.h>
#  define ZIX_MLOCK(ptr, size) VirtualLock((ptr), (size))
#else
#  pragma message("warning: No memory locking, possible RT violations")
#  define ZIX_MLOCK(ptr, size)
#endif

/*
  Note that for simplicity, only x86 and x64 are supported with MSVC.  Hopefully
  stdatomic.h support arrives before anyone cares about running this code on
  Windows on ARM.
*/
#if defined(_MSC_VER)
#  include <intrin.h>
#endif

struct ZixRingImpl {
  ZixAllocator* allocator;  ///< User allocator
  uint32_t      write_head; ///< Read index into buf
  uint32_t      read_head;  ///< Write index into buf
  uint32_t      size;       ///< Size (capacity) in bytes
  uint32_t      size_mask;  ///< Mask for fast modulo
  char*         buf;        ///< Contents
};

static inline uint32_t
zix_atomic_load(const uint32_t* const ptr)
{
#if defined(_MSC_VER)
  const uint32_t val = *ptr;
  _ReadBarrier();
  return val;
#else
  return __atomic_load_n(ptr, __ATOMIC_ACQUIRE);
#endif
}

static inline void
zix_atomic_store(uint32_t* const ptr, // NOLINT(readability-non-const-parameter)
                 const uint32_t  val)
{
#if defined(_MSC_VER)
  _WriteBarrier();
  *ptr = val;
#else
  __atomic_store_n(ptr, val, __ATOMIC_RELEASE);
#endif
}

static inline uint32_t
next_power_of_two(uint32_t size)
{
  // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
  size--;
  size |= size >> 1U;
  size |= size >> 2U;
  size |= size >> 4U;
  size |= size >> 8U;
  size |= size >> 16U;
  size++;
  return size;
}

ZixRing*
zix_ring_new(ZixAllocator* const allocator, uint32_t size)
{
  ZixRing* ring = (ZixRing*)zix_malloc(allocator, sizeof(ZixRing));

  if (ring) {
    ring->allocator  = allocator;
    ring->write_head = 0;
    ring->read_head  = 0;
    ring->size       = next_power_of_two(size);
    ring->size_mask  = ring->size - 1;

    if (!(ring->buf = (char*)zix_malloc(allocator, ring->size))) {
      zix_free(allocator, ring);
      return NULL;
    }
  }

  return ring;
}

void
zix_ring_free(ZixRing* ring)
{
  if (ring) {
    zix_free(ring->allocator, ring->buf);
    zix_free(ring->allocator, ring);
  }
}

void
zix_ring_mlock(ZixRing* ring)
{
  ZIX_MLOCK(ring, sizeof(ZixRing));
  ZIX_MLOCK(ring->buf, ring->size);
}

void
zix_ring_reset(ZixRing* ring)
{
  ring->write_head = 0;
  ring->read_head  = 0;
}

/*
  General pattern for public thread-safe functions below: start with a single
  atomic load of the "other's" index, then do whatever work, and finally end
  with a single atomic store to "your" index (if it is changed).
*/

static inline uint32_t
read_space_internal(const ZixRing* ring, uint32_t r, uint32_t w)
{
  return (w - r) & ring->size_mask;
}

uint32_t
zix_ring_read_space(const ZixRing* ring)
{
  const uint32_t w = zix_atomic_load(&ring->write_head);

  return read_space_internal(ring, ring->read_head, w);
}

static inline uint32_t
write_space_internal(const ZixRing* ring, uint32_t r, uint32_t w)
{
  return (r - w - 1U) & ring->size_mask;
}

uint32_t
zix_ring_write_space(const ZixRing* ring)
{
  const uint32_t r = zix_atomic_load(&ring->read_head);

  return write_space_internal(ring, r, ring->write_head);
}

uint32_t
zix_ring_capacity(const ZixRing* ring)
{
  return ring->size - 1;
}

static inline uint32_t
peek_internal(const ZixRing* ring,
              uint32_t       r,
              uint32_t       w,
              uint32_t       size,
              void*          dst)
{
  if (read_space_internal(ring, r, w) < size) {
    return 0;
  }

  if (r + size < ring->size) {
    memcpy(dst, &ring->buf[r], size);
  } else {
    const uint32_t first_size = ring->size - r;
    memcpy(dst, &ring->buf[r], first_size);
    memcpy((char*)dst + first_size, &ring->buf[0], size - first_size);
  }

  return size;
}

uint32_t
zix_ring_peek(ZixRing* ring, void* dst, uint32_t size)
{
  const uint32_t w = zix_atomic_load(&ring->write_head);

  return peek_internal(ring, ring->read_head, w, size, dst);
}

uint32_t
zix_ring_read(ZixRing* ring, void* dst, uint32_t size)
{
  const uint32_t w = zix_atomic_load(&ring->write_head);
  const uint32_t r = ring->read_head;
  if (!peek_internal(ring, r, w, size, dst)) {
    return 0;
  }

  zix_atomic_store(&ring->read_head, (r + size) & ring->size_mask);
  return size;
}

uint32_t
zix_ring_skip(ZixRing* ring, uint32_t size)
{
  const uint32_t w = zix_atomic_load(&ring->write_head);
  const uint32_t r = ring->read_head;
  if (read_space_internal(ring, r, w) < size) {
    return 0;
  }

  zix_atomic_store(&ring->read_head, (r + size) & ring->size_mask);
  return size;
}

uint32_t
zix_ring_write(ZixRing* ring, const void* src, uint32_t size)
{
  const uint32_t r = zix_atomic_load(&ring->read_head);
  const uint32_t w = ring->write_head;
  if (write_space_internal(ring, r, w) < size) {
    return 0;
  }

  const uint32_t end = w + size;
  if (end <= ring->size) {
    memcpy(&ring->buf[w], src, size);
    zix_atomic_store(&ring->write_head, end & ring->size_mask);
  } else {
    const uint32_t size1 = ring->size - w;
    const uint32_t size2 = size - size1;
    memcpy(&ring->buf[w], src, size1);
    memcpy(&ring->buf[0], (const char*)src + size1, size2);
    zix_atomic_store(&ring->write_head, size2);
  }

  return size;
}
