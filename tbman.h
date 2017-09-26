/**
Author & Copyright (C) 2017 Johannes Bernhard Steffens.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef TBMAN_H
#define TBMAN_H

#include <stddef.h>
#include <stdbool.h>

typedef struct tbman_s tbman_s;

/// Creates a dedicated manager with default parameters
tbman_s* tbman_s_create_default();

/// Creates a dedicated manager with specified parameters (consider using tbman_s_create_default)
tbman_s* tbman_s_create
         (
            size_t pool_size,        // size of a memory pool in a token manager
            size_t min_block_size,   // minimal block size
            size_t max_block_size,   // maximal block size
            size_t stepping_method,  // 1: uses power-2 block size stepping; > 1 uses more fine grained stepping
            bool full_align          // true: uses full memory alignment (fastest)
         );

/// opens global memory manager (call this once before first usage of global tbman functions below)
void tbman_open();

/// closes global memory manager (call this once at the end of your program)
void tbman_close();

/**********************************************************************************************************************/
/** Advanced memory management using the internal manager (thread-safe).
 *  This function provides allocation, re-allocation and freeing of memory
 *  with advanced controls to improve memory efficiency.
 *  In this context, a free request is represented as re-allocation with requested_size == 0
 *
 *  Arguments
 *    current_ptr:
 *      Pointer to current memory location:
 *        ==NULL for pure-allocation
 *        !=NULL for re-allocation or freeing
 *
 *    current_size:
 *      Optional information to memory manager about last reserved or requested amount.
 *      Allowed values: 0 or previously requested or reserved amount.
 *                      0 makes the function ignore current_ptr (assumes it is NULL)
 *
 *    requested_size:
 *       > 0 for pure-allocation or re-allocation
 *      == 0 for freeing
 *
 *    granted_size
 *      Memory granted to requester.
 *      The memory manager grants at least the requested amount of bytes. But it may grant more memory.
 *      The requester may use granted memory without re-allocation. (E.g. for dynamic arrays.)
 *      Retrieving the granted amount is optional. Use NULL when not desired.
 *
 *  Return
 *    Allocated memory address. NULL in case all memory was freed.
 *
 *  Alignment: (default behavior)
 *    A request of size of n*m bytes, where n,m are positive integers and m is (largest possible)
 *    integer power of 2, returns an address aligned to the lesser of m and TBMAN_ALIGN
 *    (TBMAN_ALIGN is defined in tbman.c).
 *    This provides correct alignment of standard data types but also for composite types
 *    (e.g. int32_4_t) for use with a SIMD extension of the CPU (e.g. Intel's SSE or ARM's Neon).
 *
 */
void* tbman_alloc(                void* current_ptr,                      size_t requested_size, size_t* granted_size );
void* tbman_nalloc(               void* current_ptr, size_t current_size, size_t requested_size, size_t* granted_size );
void* tbman_s_alloc(  tbman_s* o, void* current_ptr,                      size_t requested_size, size_t* granted_size );
void* tbman_s_nalloc( tbman_s* o, void* current_ptr, size_t current_size, size_t requested_size, size_t* granted_size );

/// malloc, free and realloc (thread-safe).
static inline void* tbman_malloc(             size_t size ) { return tbman_alloc( NULL, size, NULL ); }
static inline void* tbman_realloc( void* ptr, size_t size ) { return tbman_alloc( ptr,  size, NULL ); }
static inline void  tbman_free(    void* ptr              ) {        tbman_alloc( ptr,  0,    NULL ); }

/// realloc, specifying current size (thread-safe).
static inline void* tbman_nrealloc( void* current_ptr, size_t current_size, size_t new_size )
{
    return tbman_nalloc( current_ptr, current_size, new_size, NULL );
}

/// free, specifying current size (thread-safe).
static inline void tbman_nfree( void* current_ptr, size_t current_size )
{
    tbman_nalloc( current_ptr, current_size, 0, NULL );
}

/// returns total of currently granted space (thread-safe)
size_t tbman_granted_space( void );
size_t tbman_s_granted_space( tbman_s* o );

/// prints internal status to stdout (use only for debugging/testing - not thread-safe)
void print_tbman_s_status( tbman_s* o, int detail_level );
void print_tbman_status(               int detail_level );

#endif // TBMAN_H
