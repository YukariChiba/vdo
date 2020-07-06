/*
 * Copyright (c) 2020 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA. 
 *
 * $Id: //eng/uds-releases/krusty/src/uds/memoryAlloc.h#5 $
 */

#ifndef MEMORY_ALLOC_H
#define MEMORY_ALLOC_H 1

#include <stdarg.h>

#include "compiler.h"
#include "cpu.h"
#include "permassert.h"
#include "typeDefs.h"

#ifdef __KERNEL__
#include <linux/io.h>  // for PAGE_SIZE
#include "threadRegistry.h"
#endif

/**
 * Allocate storage based on memory size and  alignment, logging an error if
 * the allocation fails. The memory will be zeroed.
 *
 * @param size   The size of an object
 * @param align  The required alignment
 * @param what   What is being allocated (for error logging)
 * @param ptr    A pointer to hold the allocated memory
 *
 * @return UDS_SUCCESS or an error code
 **/
int __must_check
allocate_memory(size_t size, size_t align, const char *what, void *ptr);

/**
 * Free storage
 *
 * @param ptr  The memory to be freed
 **/
void free_memory(void *ptr);

/**
 * Allocate storage based on element counts, sizes, and alignment.
 *
 * This is a generalized form of our allocation use case: It allocates
 * an array of objects, optionally preceded by one object of another
 * type (i.e., a struct with trailing variable-length array), with the
 * alignment indicated.
 *
 * Why is this inline?  The sizes and alignment will always be
 * constant, when invoked through the macros below, and often the
 * count will be a compile-time constant 1 or the number of extra
 * bytes will be a compile-time constant 0.  So at least some of the
 * arithmetic can usually be optimized away, and the run-time
 * selection between allocation functions always can.  In many cases,
 * it'll boil down to just a function call with a constant size.
 *
 * @param count   The number of objects to allocate
 * @param size    The size of an object
 * @param extra   The number of additional bytes to allocate
 * @param align   The required alignment
 * @param what    What is being allocated (for error logging)
 * @param ptr     A pointer to hold the allocated memory
 *
 * @return UDS_SUCCESS or an error code
 **/
static INLINE int doAllocation(size_t      count,
                               size_t      size,
                               size_t      extra,
                               size_t      align,
                               const char *what,
                               void       *ptr)
{
  size_t totalSize = count * size + extra;
  // Overflow check:
  if ((size > 0) && (count > ((SIZE_MAX - extra) / size))) {
    /*
     * This is kind of a hack: We rely on the fact that SIZE_MAX would
     * cover the entire address space (minus one byte) and thus the
     * system can never allocate that much and the call will always
     * fail.  So we can report an overflow as "out of memory" by asking
     * for "merely" SIZE_MAX bytes.
     */
    totalSize = SIZE_MAX;
  }

  return allocate_memory(totalSize, align, what, ptr);
}

/**
 * Reallocate dynamically allocated memory.  There are no alignment guarantees
 * for the reallocated memory.
 *
 * @param ptr      The memory to reallocate.
 * @param oldSize  The old size of the memory
 * @param size     The new size to allocate
 * @param what     What is being allocated (for error logging)
 * @param newPtr   A pointer to hold the reallocated pointer
 *
 * @return UDS_SUCCESS or an error code
 **/
int __must_check reallocate_memory(void *ptr,
				   size_t oldSize,
				   size_t size,
				   const char *what,
				   void *newPtr);

/**
 * Allocate one or more elements of the indicated type, logging an
 * error if the allocation fails. The memory will be zeroed.
 *
 * @param COUNT  The number of objects to allocate
 * @param TYPE   The type of objects to allocate.  This type determines the
 *               alignment of the allocated memory.
 * @param WHAT   What is being allocated (for error logging)
 * @param PTR    A pointer to hold the allocated memory
 *
 * @return UDS_SUCCESS or an error code
 **/
#define ALLOCATE(COUNT, TYPE, WHAT, PTR) \
  doAllocation(COUNT, sizeof(TYPE), 0, __alignof__(TYPE), WHAT, PTR)

/**
 * Allocate one object of an indicated type, followed by one or more
 * elements of a second type, logging an error if the allocation
 * fails. The memory will be zeroed.
 *
 * @param TYPE1  The type of the primary object to allocate.  This type
 *               determines the alignment of the allocated memory.
 * @param COUNT  The number of objects to allocate
 * @param TYPE2  The type of array objects to allocate
 * @param WHAT   What is being allocated (for error logging)
 * @param PTR    A pointer to hold the allocated memory
 *
 * @return UDS_SUCCESS or an error code
 **/
#define ALLOCATE_EXTENDED(TYPE1, COUNT, TYPE2, WHAT, PTR)             \
  __extension__ ({                                                    \
      TYPE1 **_ptr = (PTR);                                           \
      STATIC_ASSERT(__alignof__(TYPE1) >= __alignof__(TYPE2));        \
      int _result = doAllocation(COUNT, sizeof(TYPE2), sizeof(TYPE1), \
                                 __alignof__(TYPE1), WHAT, _ptr);     \
      _result;                                                        \
    })

/**
 * Allocate one or more elements of the indicated type, aligning them
 * on the boundary that will allow them to be used in I/O, logging an
 * error if the allocation fails. The memory will be zeroed.
 *
 * @param COUNT  The number of objects to allocate
 * @param TYPE   The type of objects to allocate
 * @param WHAT   What is being allocated (for error logging)
 * @param PTR    A pointer to hold the allocated memory
 *
 * @return UDS_SUCCESS or an error code
 **/
#ifdef __KERNEL__
#define ALLOCATE_IO_ALIGNED(COUNT, TYPE, WHAT, PTR) \
  doAllocation(COUNT, sizeof(TYPE), 0, PAGE_SIZE, WHAT, PTR)
#else
#define ALLOCATE_IO_ALIGNED(COUNT, TYPE, WHAT, PTR) \
  ALLOCATE(COUNT, TYPE, WHAT, PTR)
#endif

/**
 * Free memory allocated with ALLOCATE().
 *
 * @param ptr    Pointer to the memory to free
 **/
static INLINE void FREE(void *ptr)
{
  free_memory(ptr);
}

/**
 * Allocate memory starting on a cache line boundary, logging an error if the
 * allocation fails. The memory will be zeroed.
 *
 * @param size  The number of bytes to allocate
 * @param what  What is being allocated (for error logging)
 * @param ptr   A pointer to hold the allocated memory
 *
 * @return UDS_SUCCESS or an error code
 **/
static INLINE int __must_check
allocateCacheAligned(size_t size, const char *what, void *ptr)
{
  return allocate_memory(size, CACHE_LINE_BYTES, what, ptr);
}

#ifdef __KERNEL__
/**
 * Allocate storage based on memory size, failing immediately if the required
 * memory is not available.  The memory will be zeroed.
 *
 * @param size  The size of an object.
 * @param what  What is being allocated (for error logging)
 *
 * @return pointer to the allocated memory, or NULL if the required space is
 *         not available.
 **/
void * __must_check allocate_memory_nowait(size_t size, const char *what);

/**
 * Allocate one element of the indicated type immediately, failing if the
 * required memory is not immediately available.
 *
 * @param TYPE   The type of objects to allocate
 * @param WHAT   What is being allocated (for error logging)
 *
 * @return pointer to the memory, or NULL if the memory is not available.
 **/
#define ALLOCATE_NOWAIT(TYPE, WHAT) allocate_memory_nowait(sizeof(TYPE), WHAT)
#endif

/**
 * Duplicate a string.
 *
 * @param string    The string to duplicate
 * @param what      What is being allocated (for error logging)
 * @param newString A pointer to hold the duplicated string
 *
 * @return UDS_SUCCESS or an error code
 **/
int __must_check
duplicateString(const char *string, const char *what, char **newString);

/**
 * Duplicate a buffer, logging an error if the allocation fails.
 *
 * @param ptr     The buffer to copy
 * @param size    The size of the buffer
 * @param what    What is being duplicated (for error logging)
 * @param dupPtr  A pointer to hold the allocated array
 *
 * @return UDS_SUCCESS or ENOMEM
 **/
int __must_check
memdup(const void *ptr, size_t size, const char *what, void *dupPtr);

/**
 * Wrapper which permits freeing a const pointer.
 *
 * @param pointer  the pointer to be freed
 **/
static INLINE void freeConst(const void *pointer)
{
  union {
    const void *constP;
    void *notConst;
  } u = { .constP = pointer };
  FREE(u.notConst);
}

/**
 * Wrapper which permits freeing a volatile pointer.
 *
 * @param pointer  the pointer to be freed
 **/
static INLINE void freeVolatile(volatile void *pointer)
{
  union {
    volatile void *volP;
    void *notVol;
  } u = { .volP = pointer };
  FREE(u.notVol);
}

#ifdef __KERNEL__
/**
 * Perform termination of the memory allocation subsystem.
 **/
void memory_exit(void);

/**
 * Perform initialization of the memory allocation subsystem.
 **/
void memory_init(void);

/**
 * Register the current thread as an allocating thread.
 *
 * An optional flag location can be supplied indicating whether, at
 * any given point in time, the threads associated with that flag
 * should be allocating storage.  If the flag is false, a message will
 * be logged.
 *
 * If no flag is supplied, the thread is always allowed to allocate
 * storage without complaint.
 *
 * @param new_thread  registered_thread structure to use for the current thread
 * @param flag_ptr    Location of the allocation-allowed flag
 **/
void register_allocating_thread(struct registered_thread *new_thread,
                                const bool               *flag_ptr);

/**
 * Unregister the current thread as an allocating thread.
 **/
void unregister_allocating_thread(void);

/**
 * Get the memory statistics.
 *
 * @param bytesUsed     A pointer to hold the number of bytes in use
 * @param peakBytesUsed A pointer to hold the maximum value bytesUsed has
 *                      attained
 **/
void get_memory_stats(uint64_t *bytesUsed, uint64_t *peakBytesUsed);

/**
 * Report stats on any allocated memory that we're tracking.
 *
 * Not all allocation types are guaranteed to be tracked in bytes
 * (e.g., bios).
 **/
void report_memory_usage(void);

#endif

#endif /* MEMORY_ALLOC_H */
