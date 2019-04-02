# Tbman - Fast and Easy Memory Manager

# Table of Content
   * [What it is](#anchor_what_it_is)
   * [Benefits](#anchor_benefits)
   * [How to use it](#anchor_how_to_use_it)
   * [Detailed description](#anchor_features)
      * [Basics](#anchor_basic)
      * [Faster collection](#anchor_faster_collection)
      * [One function for everything](#anchor_one_function_for_everything)
      * [Automatic alignment](#anchor_automatic_alignment)
      * [Granted amount](#anchor_granted_amount)
      * [Memory tracking](#anchor_memory_tracking)
      * [Multiple managers](#anchor_multiple_managers)
   * [How it works](#anchor_how_it_works)
      * [Block-Pooling-Layer](#anchor_block-pooling-layer)
      * [Thread safety](#anchor_thread_safety)
      * [Mixing different memory managers](#anchor_mixing_different_memory_managers)
   * [Potential downsides](#potential_downsides)      
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
* Very fast. [*You can easily verify this yourself.*](#anchor_quick_evaluation)
* [Automatic alignment](#anchor_automatic_alignment) even for extended data types. *(E.g. SIMD types such as `int32x4_t`)*
* Fewer system calls.
* Low fragmentation of system memory.
* [Easy monitoring](#anchor_memory_tracking) of total memory usage. *(E.g. for leak detection)*
* Optional communicaton of [actually granted amounts](#anchor_granted_amount). *(Useful for dynamic arrays)*
* Platform independence:
   * The code can be built on any platform satisfying the [build requirements](#anchor_build_requirements) below.
   * It has been tested on Intel and ARM platforms.

<a name="anchor_how_to_use_it"></a>
## How to use it
* `git clone https://github.com/johsteffens/tbman.git`

<a name="anchor_build_requirements"></a>
### Requirements/Dependencies
   * Compiler suite supporting the C11 standard (e.g. gcc, clang).
   * Some POSIX compliance (e.g. pthread should be available).
      * Linux, Android, Darwin (and related OS) normally comply sufficiently.
      * Windows (general): [Setting up a POSIX-environment is possible.](https://github.com/johsteffens/beth/wiki/Requirements#how-to-setup-a-posix-environment-for-beth-on-windows)
      * Windows 10: Also provides an optional Linux-Subsystem.

   * Compiler options: `-std=c11 -O3`
   * Linker options: `-lm -lpthread`
   
### In your workspace
* Compile `tbman.c` and `btree.c` (either among your source files or into a static library)
* In your code:
  * `#include "tbman.h"`
  * Call once `tbman_open();` at the beginning or your program. *(E.g. first in `main()`)*
  * Use `tbman_*` - functions anywhere.
  * Call once `tbman_close();` at the end or your program. *(E.g. last in `main()`)*

### C++
In object oriented C++ programming, the direct use of `malloc`, `realloc` or `free` is discouraged in favor of using operators `new` and `delete`, which take care of object construction/destruction. However, you can overload these operators, taking control over the part concerned with memory allocation.

**Example:**
```C++
void* operator new( size_t size ) { return tbman_malloc( size ); }
void operator detete( void* p ) { tbman_free( p ); }
```
   
Here is an external article with more details about overloading allocation operators:<br>
http://www.modernescpp.com/index.php/overloading-operator-new-and-delete

<a name="anchor_quick_evaluation"></a>
### Quick evaluation

`eval.c` is an evaluation program simulating realistic runtime conditions for a memory manager. It verifies correct functionality and assesses the processing speed. It compares the performance of stdlib functions with tbman functions. You can quickly run it yourself:
   * Enter folder with source files and run: `gcc -std=c11 -O3 btree.c tbman.c eval.c -lm -lpthread; ./a.out`

<a name="anchor_features"></a>
## Detailed description

<a name="anchor_basic"></a>
### Basics
Tbman offers the three basic functions of a memory manager:
```C
void* tbman_malloc(             size_t size ); // new allocation
void* tbman_realloc( void* ptr, size_t size ); // reallocation
void  tbman_free(    void* ptr              ); // freeing
```
Usage and behavior is compatible to corresponding stdlib functions `malloc`, `free`, `realloc`.
<br><sub>Exception: Should the entire system run out of available memory, tbman aborts with an error message to stderr.</sub>

Tbman must be initialized once before usage and should also be properly exited at the end of the program:
```C
void tbman_open( void );  // initializes tbman
void tbman_close( void ); // closes tbman
```
**Example:**
```C 
int main( int argc, char* argv[] )
{
   tbman_open();
   
   ... // my program
   
   tbman_close();
   return my_exit_state;
}
```

<a name="anchor_faster_collection"></a>
### Faster collection
If you free or reallocate memory and know the previoulsy allocated amount, you can further speed up processing by telling tbman about the currently allocated size using `tbman_nrealloc` for `tbman_realloc` and `tbman_nfree` for `tbman_free`. This helps the manager find the corresponding node for the memory instance.

```C
void* tbman_nrealloc( void* current_ptr, size_t current_size, size_t new_size );
void  tbman_nfree(    void* current_ptr, size_t current_size );
```
`current_size` must hold either the requested amount or the [granted amount](#anchor_granted_amount) for the memory instance addressed by `current_ptr`.

<a name="anchor_one_function_for_everything"></a>
### One function for everything
Alternatively, you can use one of the following two functions to handle all basic manager functionality as well as some special features of tbman.

```C 
void* tbman_alloc(  void* current_ptr,                      size_t requested_size, size_t* granted_size );
void* tbman_nalloc( void* current_ptr, size_t current_size, size_t requested_size, size_t* granted_size );
```
`tbman_nalloc` works slightly faster than `tbman_alloc` but requires extra size input. The two functions can also be mixed; even serving the same memory instance.

**Arguments**
   * `current_ptr` <br>
Pointer to current memory instance for freeing or reallocating; Set to `NULL` for pure allocation.

   * `current_size` (only `tbman_nalloc`) <br>
Previously requested or [granted size](#anchor_granted_amount) for freeing or reallocating a memory instance. Set to `0` for pure allocation.  

   * `requested_size` <br>
Requested new size pure allocation or reallocation. Set to `0` for freeing.

   * `granted_size` <br>
Optional pointer to variable where the function stores the [granted amount](#anchor_granted_amount). Set to `NULL` when not needed.

**Return value** <br>
Pointer to new memory instance for pure allocation or reallocation. Returns `NULL` in case of freeing.

(See also inline documentation for these functions in [`tbman.h`](https://github.com/johsteffens/tbman/blob/947a88c820943c9902e572cb0c301f75daaae45e/tbman.h#L101)).

<a name="anchor_automatic_alignment"></a>
### Automatic alignment
Tbman aligns the memory instance. This covers standard C/C++ data types `char, short, int, float, double, etc` and also larger types such as `int32x4_t, float32x4_t, etc`, which are typically used for SIMD-extensions such as `SSE, AVX, NEON, etc`.

To achieve this, tbman analyzes the requested size. If you allocate an instance or array of type `my_type` with `sizeof( my_type )` being a power of two not larger than [`TBMAN_ALIGN`](https://github.com/johsteffens/tbman/blob/848bebed1648d66d1fe101ee19f4803fed8ea81a/tbman.c#L43), then the memory block is alinged to `sizeof( my_type )`. More generally: When requesting memory of _**s**_ bytes and _**s**_ can be expressed as product of two positive integers _**s**_ = _**m**\***n**_ such that _**m**_ is a power of 2, then the returned memory is aligned to the lesser of _**m**_ and [`TBMAN_ALIGN`](https://github.com/johsteffens/tbman/blob/848bebed1648d66d1fe101ee19f4803fed8ea81a/tbman.c#L43). 

**Example:**
```C 
int32x4_t* my_data = tbman_malloc( sizeof( int32x4_t ) * 10 ); // aligned array of 10 x int32x4_t
```

<a name="anchor_granted_amount"></a>
### Granted amount
For design reasons tbman might find no proper use for some space immediately following your requested memory block. In that case it grants you that extra space, appending it to your request. You may use that granted space as if you had requested that larger space in the first place. 
<br><sub>*(Note: Tbman never grants less than requested.)*</sub>

Knowing about the the granted amount can be useful e.g. when optimizing the behavior of dynamic arrays. The following functions communicate the granted amount in bytes:

   * Explicit query for the granted amount for an already existing memory instance:
```C 
size_t tbman_granted_space( const void* ptr );
```

   * [Allocation with granted amount communicated](#anchor_one_function_for_everything):
```C 
void* tbman_alloc( void* current_ptr, size_t requested_size, size_t* granted_size );
```
**Example:**
```C 
size_t requested_space = 5;
size_t granted_space;
char* my_string = tbman_alloc( NULL, requested_space, &granted_space );
// At this point granted_space >= requested_space. Using that extra space is allowed.
for( size_t i = 0; i < requested_space - 1; i++ ) my_string[ i ] = '=';
for( size_t i = requested_space - 1; i < granted_space - 1; i++ ) my_string[ i ] = '#';
my_string[ granted_space - 1 ] = 0;
printf( "%s\n", my_string );
// Possible output: ====###
```

<a name="anchor_memory_tracking"></a>
### Memory tracking
You can query the total of tbman-allocated memory at any point in your program. The following function does this:
```C 
size_t tbman_total_granted_space( void );
```
This can be helpful to asses the memory footprint of your code and for leak detection.

**Example:**
```C 
size_t prior_space = tbman_total_granted_space();
my_never_leaking_function( arg1, arg2, arg3 );
size_t memory_leak = tbman_total_granted_space() - prior_space;
if( memory_leak > 0 ) fprintf( stderr, "Memory leak of %zu bytes detected.\n", memory_leak );
```
<a name="anchor_multiple_managers"></a>
### Multiple managers
Functions `tbman_` above relate to global management (one manager for everything). You can also create multiple individual, independent and dedicated managers using the the `tbman_s` object. Each manager has its own mutex. This is particularly helpful in a multi threaded context. Giving each thread its own manager for thread-local memory can reduce lock-contentaion.

For each of above functions `tbman_` there exists a corresponding function with postfix `_s` meant for a dedicated manager instance. Except `tbman_s_open`, all functions `tbman_s_` take as first argument the reference to the dedicated manager instance.

**Example:**
```C 
tbman_s* my_man = tbman_s_open(); // opens a dedicated manager
char* my_memory = tbman_s_malloc( my_man, 1024 );
... // do something else
tbman_s_free( my_man, my_memory );  
tbman_s_close( my_man ); // closes a dedicated manager
```
<a name="anchor_how_it_works"></a>
## How it works

<a name="anchor_block-pooling-layer"></a>
### Block-Pooling-Layer with Tokens
Tbman introduces a dedicated management layer using a "conservative" memory pooling with multiple fixed size block-managers at a strategic size-distribution. Multiple pools are managed in a btree. When the client (your code) requests or returns small-medium sized memory instances, tbman dispatches/recollects pool memory accordingly without initiating system requests. System requests are executed infrequently to acquire a new pool or return an empty pool. This offloads the system manager significantly. Compared to always using system calls it can speed up overall processing and/or reduce fragmentation, particularly in programs where many small sized memory instances are used.

Each memory instance is associated with an internal node controlled by tbman. The manager dedicates separate memory areas for node-control and user space (== memory space used by the client). The content of user space does not affect node management. Hence, specific software bugs such as using a dangling pointer (pointer to already collected memory) are less likely to mess up the manager itself and can be more easily tracked down.

A special design feature is the combination of associative tokens with a special alignment scheme. It provides quick binding of memory address and manager-nodes. This method ensures very low latency for allocation and collection and it gives this manager its name: tbman = token-block-manager.

When the client requests a large memory instance, where pooling would be wasteful, tbman falls back to using a direct system call. However, it [keeps track](#anchor_memory_tracking) of all memory.

<a name="anchor_thread_safety"></a>
### Thread safety
Tbman is thread safe: The interface functions can be called any time from any thread simultaneously. Memory allocated in one thread can be freed in any other thread.

Concurrency is governed by a mutex. This means that memory management is not lock free. Normally, this will not significantly affect processing speed for typical multi threaded programs. Only during heavvy simultaneous usage of the same manager lock-contention time might be noticeable compared to single threaded usage.

<a name="anchor_mixing_different_memory_managers"></a>
### Mixing different memory managers
Tbman does not affect the behavior of other memory managers (such as `malloc`, `free`, `realloc`), so you can mix code using different management systems.

However, different managers can not serve the **same memory instance**. For example: You can not use `free` or `realloc` on a memory instance which was allocated with `tbman_malloc` (`tbman_realloc`) or vice versa.

Likewise, you can not manage the same memory instance with different [dedicated managers](#anchor_multiple_managers).

<a name="potential_downsides"></a>
## Potential downsides

### Preallocations
Tbman reserves and returns system memory in larger chunks to offload the system. That means that the memory your application reserves at a given time is likely higher than if you use system functions directly. In practice, this overhead is usually tolerable but you might want to be aware of it.

### Memory slots with predefined size distribution
Tbman organizes memory instances into slots with a predefined size distribution. For an allocation request the best fitting slot is selected and the full slot-size is [granted](#anchor_granted_amount). If you do not need that extra amount, it is wasted. Tests have shown that in realistic situations this extra size tends to average around 20% of total memory usage.

### Unsuitable memory model
Tbman makes an assumption about the system's memory model:
   * If two pointers `ptr1`, `ptr2` reference valid but **different** objects anywhere in the application's addressable memory space, then `( ptrdiff_t )( ptr1 - ptr2 )` can never be zero.

Although this sounds like a no-brainer, it actually goes beyond the standard C provisions. Std. C allows the compiler implementation to leave the result of pointer subtraction undefined if the objects are not of the same array or same host-object. (see [cppreference.com: Pointer arithmetic](https://en.cppreference.com/w/c/language/operator_arithmetic#Pointer_arithmetic).)

However, most modern platforms employ a flat memory model where tbman's assumption is correct and safe. Very old systems, like early x86 platforms, use a segmented memory model (segment:offset) where only the offset participates in pointer arithmetic, thus thwarting the assumption.

<a name="anchor_motivation"></a>
## Motivation
This memory manager has originally been conceived and developed for the project [beth](https://github.com/johsteffens/beth). For those interested in the manager but not keen on digesting the whole of project beth, we created this compact stand-alone solution as spin-off-project [tbman](https://github.com/johsteffens/tbman).

Thanks for reading. If you find it useful, have questions or suggestions I'd be happy to hear from you.

<sub>*Johannes Steffens*</sub>
