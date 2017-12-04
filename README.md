# Tbman - Fast and Easy Memory Manager

## What it is
Tbman is a general-purpose memory manager offering (among others) these functions 

`tbman_malloc, tbman_free, tbman_realloc`,

which can replace corresponding stdlib functions

`malloc, free, realloc`,

in C and C++ code.

## Benefits
* Generally faster than stdlib functions. [*You can quickly verify this yourself.*](#anchor_quick_evaluation)
* Automatic alignment even for extended data types. *(E.g. SIMD types such as `int32x4_t`)*
* Reduced fragmentation of system memory.
* Easy monitoring of total memory usage. *(E.g. for leak detection)*
* Optional communicaton of actually granted amounts. *(Useful for dynamic arrays)*
* Platform independence: 
   * The code adheres to the c11-standard.
   * It can be built on any platform satisfying the [build requirements](#anchor_build_requirements) below.
   * It has been tested on Intel and ARM platforms.

## How to use it
<a name="anchor_quick_evaluation"></a>
**Quick evaluation**:

`eval.c` is an evaluation program simulating realistic runtime conditions for a memory manager. It verifies correct functionality and assesses the processing speed. It compares the performance of stdlib functions with tbman functions. You can quickly run it yourself:
   * Download this project.
   * (Linux) Enter folder with source files and run: `gcc -std=c11 -O3 btree.c tbman.c eval.c -lm -lpthread; ./a.out`

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
* Your C-comiler should support the C11 standard: `-std=c11`
* Tbman uses the pthread library: `-lpthread`

**C++:**

You can use tbman in C++ code as follows:
* Include tbman.h as C-header:<br>
  ```C++
  extern "C"
  {
     #include "tbman.h"
  }
  ```
* In object oriented C++ programming, the direct use of `malloc`, `realloc` or `free` is discouraged in favor of using operators `new` and `delete`, which take care of object construction/destruction. However, you can overload these operators, taking control over the part concerned with memory allocation.<br>
**Example:**
    ```C++
    void* operator new( size_t size ) { return tbman_malloc( size ); }
    void operator detete( void* p ) { tbman_free( p ); }
    ```
    Here is a nice external article about overloading allocation operators:<br>
    http://www.modernescpp.com/index.php/overloading-operator-new-and-delete
   
## How it works
Tbman uses a "conservative" memory pooling approach with multiple token-based fixed size block-managers at a strategic size-distribution. It pre-allocates only moderate amounts of memory and dymatically acquires more or releases back to the system as needed and/or suitable. Multiple pools are managed in a btree.

For large memory requests, where pooling would be wasteful, tbman falls back to using direct system calls. However, it keeps track of all memory.

### Use in multi-threaded applications
Tbman is thread safe: The interface functions can be called any time from any thread simultaneously. Memory allocated in one thread can be freed in any other thread.

Concurrency is currently achieved by only one mutex. This means that memory management is not truly parallel. During simultaneous memory requests, one thread is put on hold until the request in the other completes. Normally this should not significantly affect processing speed for typical multi threaded programs. However, during heavvy simultaneous manager usage, the lock-time might accumulate to a signficant share.

## Motivation
This memory manager has been conceived and developed for the project [beth](https://github.com/johsteffens/beth). It has reached sufficient maturity for general purpose and platform independent usage. We therefore created the spin-off-project [tbman](https://github.com/johsteffens/tbman) where the manager is offered as compact stand-alone solution.



Thanks for reading. I hope you will find it useful.

<sub>*Johannes Steffens*</sub>
