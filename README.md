# Tbman - Fast and Easy Memory Manager

# Table of Content
   * [What it is](#anchor_what_it_is)
   * [Benefits](#anchor_benefits)
   * [How to use it](#anchor_how_to_use_it)
   * [Features](#anchor_features)
   * [How it works](#anchor_how_it_works)
   * [Motivation](#anchor_motivation)

<a name="anchor_what_it_is"></a>
## What it is
Tbman is a general-purpose memory manager offering (among others) these functions

`tbman_malloc, tbman_free, tbman_realloc`,

which can replace corresponding stdlib functions

`malloc, free, realloc`,

in C and C++ code.

<a name="anchor_benefits"></a>
## Benefits
* Very fast. [*You can quickly verify this yourself.*](#anchor_quick_evaluation)
* Automatic alignment even for extended data types. *(E.g. SIMD types such as `int32x4_t`)*
* Low fragmentation of system memory.
* Easy monitoring of total memory usage. *(E.g. for leak detection)*
* Optional communicaton of actually granted amounts. *(Useful for dynamic arrays)*
* Platform independence:
   * The code adheres to the c11-standard.
   * It can be built on any platform satisfying the [build requirements](#anchor_build_requirements) below.
   * It has been tested on Intel and ARM platforms.

<a name="anchor_how_to_use_it"></a>
## How to use it
* `git clone https://github.com/johsteffens/tbman.git`

**Operating systems supporting POSIX: (e.g. Linux, Android, Darwin, ... )** Just follow suggestions below.

**Windows:** [Set up a POSIX-environment first](https://github.com/johsteffens/beth/wiki/Requirements#how-to-setup-a-posix-environment-for-beth-on-windows).

### Requirements/Dependencies
   * gcc (or similar compiler suite) supporting the C11 standard.
   * Library `pthread` of the POSIX.1c standard.

<a name="anchor_quick_evaluation"></a>
**Quick evaluation**:

`eval.c` is an evaluation program simulating realistic runtime conditions for a memory manager. It verifies correct functionality and assesses the processing speed. It compares the performance of stdlib functions with tbman functions. You can quickly run it yourself:
   * Enter folder with source files and run: `gcc -std=c11 -O3 btree.c tbman.c eval.c -lm -lpthread; ./a.out`

**In your workspace:**
* Compile `tbman.c` and `btree.c` (either among your source files or into a static library)
* In your code:
  * `#include "tbman.h"`
  * Call once `tbman_open();` at the beginning or your program. *(E.g. first in `main()`)*
  * Use `tbman_*` - functions anywhere.
    <br><sub>Note: Do not mix stdlib and tbman alloc-functions for the same memory instance.</sub>
  * Call once `tbman_close();` at the end or your program. *(E.g. last in `main()`)*

<a name="anchor_build_requirements"></a>
**Build requirements:**
* Your C-compiler should support the C11 standard: `-std=c11`
* Tbman uses the pthread library: `-lpthread`

**C++:**

In object oriented C++ programming, the direct use of `malloc`, `realloc` or `free` is discouraged in favor of using operators `new` and `delete`, which take care of object construction/destruction. However, you can overload these operators, taking control over the part concerned with memory allocation.

**Example:**
```C++
void* operator new( size_t size ) { return tbman_malloc( size ); }
void operator detete( void* p ) { tbman_free( p ); }
```
   
Here is an external article with more details about overloading allocation operators:<br>
http://www.modernescpp.com/index.php/overloading-operator-new-and-delete

<a name="anchor_features"></a>
## Features

### Basic
Tbman offers the basic three functions of a memory manager:
```C
void* tbman_malloc(             size_t size );
void* tbman_realloc( void* ptr, size_t size );
void  tbman_free(    void* ptr              );
```
Usage and behavior is compatible to corresponding stdlib functions `malloc`, `free`, `realloc`.
<br><sub>Exception: Should the entire system run out of available memory, tbman aborts with an error message to stderr.</sub>

### One function for everything
Alternatively, you can use one of the following two functions to handle all basic manager functionality as well as some special features of tbman. (For more details, see inline documentation for these functions in [`tbman.h`](https://github.com/johsteffens/tbman/blob/master/tbman.h)).

```C 
void* tbman_alloc( void* current_ptr, size_t requested_size, size_t* granted_size );
void* tbman_nalloc( void* current_ptr, size_t current_size, size_t requested_size, size_t* granted_size );
```

### Automatic Alignment
When requesting memory of size n\*m when n is a positive integer and m is the highest possible power of 2, then the returned memory is aligned to the lesser of m and 'TBMAN_ALIGN'. In practice this means that if you allocate an array of data type `mytype` with `sizeof( mytype )` being a power or two and <= TBMAN_ALIGN then all elements of the array are aligned to `sizeof( mytype )`. This is very helpful when your code uses SIMD vectorization and expects proper data alignment.


### Granted Amount
Tbman never grants less than requested but it might grant more. In that case the granted amount can be considered allocated. This can be utilized for example in dynamic arrays. 

   * Query for the granted amount for an existing memory instance:
```C 
size_t tbman_granted_space( const void* ptr );
```

   * Allocation with granted amount communicated. (For more details, see inline documentation for `tbman_alloc` in [`tbman.h`](https://github.com/johsteffens/tbman/blob/master/tbman.h)).
```C 
void* tbman_alloc( void* current_ptr, size_t requested_size, size_t* granted_size );
```

### Total allocated memory
You can query the total of tbman-allocated memory at any point in your program. The following function does this:
```C 
size_t tbman_total_granted_space( void );
```
This can be helpful to asses the memory footprint of your code and for leak detection.

**Example:**
```C 
size_t prior_space = tbman_total_granted_space();
my_function_that_should_never_leak( arg1, arg2, arg3 );
size_t memory_leak = tbman_total_granted_space() - prior_space;
if( memory_leak > 0 ) fprintf( stderr, "Memory leak of %zu bytes detected.\n", memory_leak );
```
### Dedicated managers
Functions `tbman_` above relate to global management (one manager for everything). You can also create multiple individual, independent and dedicated managers using the the `tbman_s` object. Each manager has its own mutex. This is particularly helpful in multiple threads to reduce lock-contention by giving each thread its own manager. 

For each of above functions `tbman_` there exists a corresponding function with postfix `_s` meant for a dedicated manager instance. Except `tbman_s_open`, all functions `tbman_s_` take as first argument the reference to the dedicated manager instance.

**Example:**
```C 
tbman_s* my_man = tbman_s_open(); // opens a dedicated manager
char* my_memory = tbman_s_malloc( my_man, 1024 );
... // do something else
tbman_s_free( my_man, my_memory );  
tbman_s_close( mman ); // closes a dedicated manager
```
<a name="anchor_how_it_works"></a>
## How it works

### Block-Pooling-Layer
Tbman introduces a dedicated management layer using a "conservative" memory pooling with multiple token-based fixed size block-managers at a strategic size-distribution. Multiple pools are managed in a btree. When the client (your code) requests or returns small-medium sized memory instances, tbman dispatches/recollects pool memory accordingly without initiating system requests. System requests are executed infrequently to acquire new pool or return an empty pool. This offloads the system manager significantly and can speed up overall processing and/or reduce fragmentation compared to always using system calls particularly in programs where many small sized memory instances are used.

For large memory requests, where pooling would be wasteful, tbman falls back to using direct system calls. However, it keeps track of all memory.

### Thread safety
Tbman is thread safe: The interface functions can be called any time from any thread simultaneously. Memory allocated in one thread can be freed in any other thread.

Concurrency is governed by a mutex. This means that memory management is not lock free. Normally, this will not significantly affect processing speed for typical multi threaded programs. Only during heavvy simultaneous usage of the same manager lock-contention time might be noticeable compared to single threaded usage.

### Mixing code using other memory managers
Tbman does not affect the behavior of other memory managers (such as `malloc`, `free`, `realloc`), so you can mix code using different management systems.

However, you can not manage the **same memory instance** with different managers. Meaning: You must use `tbman_free` or `tbman_realloc` on a memory instance allocated with `tbman_malloc` or `tbman_realloc`. You can not use `free` or `realloc` on such an instance. The opposite applies too: You can not use `tbman_free` or `tbman_realloc` on a memory instance allocated with `malloc`, `realloc`, `new` or any other non-tbman allocator.

Likewise, you can not manage the same memory instance with different dedicated managers.

<a name="anchor_motivation"></a>
## Motivation
This memory manager has originally been conceived and developed for the project [beth](https://github.com/johsteffens/beth). For those interested in the manager but not keen on digesting the whole of project beth, we created this compact stand-alone solution as spin-off-project [tbman](https://github.com/johsteffens/tbman).

Thanks for reading. If you find it useful, have questions or suggestions I'd be happy to hear from you.

<sub>*Johannes Steffens*</sub>
