# Tbman - Fast and Easy Memory Manager

## Table of Content

   * [What it is](#anchor_what_it_is)
   * [Benefits](#anchor_benefits)
   * [How to use it](#anchor_how_to_use_it)
   * [Detailed Description](#anchor_features)
      * [Basics](#anchor_basic)
      * [Faster collection](#anchor_faster_collection)
      * [One function for everything](#anchor_one_function_for_everything)
      * [Automatic Alignment](#anchor_automatic_alignment)
      * [Granted Memory](#anchor_granted_memory)
      * [Diagnostic Features](#anchor_diagnostic_features)
      * [Thread safety](#anchor_thread_safety)
      * [Multiple managers](#anchor_multiple_managers)
      * [Mixing different memory managers](#anchor_mixing_different_memory_managers)
   * [Side effects](#anchor_side_effects)
      * [Prefetching](#anchor_prefetching)
      * [Unused Memory](#anchor_unused_memory)
      * [Memory Model](#anchor_memory_model)
      * [Debugging Tools](#anchor_debugging_tools)
   * [How it works internally](#anchor_how_it_works_internally)
   * [Motivation/Origin](#anchor_motivation)

<a name="anchor_what_it_is"></a>
# What it is

Tbman is a general-purpose memory manager offering (among others) these functions

`tbman_malloc, tbman_free, tbman_realloc`,

which can replace corresponding stdlib functions

`malloc, free, realloc`,

in C and C++ code.

<a name="anchor_benefits"></a>
# Benefits

* Very fast. ([*You can easily verify this yourself.*](#anchor_quick_evaluation))
* Low fragmentation of system memory.
* Few system calls.
* [Improved Alignment](#anchor_automatic_alignment).
* [Integrated Leak Detection](#anchor_integrated_leak_detection)
* [Diagnostic Features](#anchor_diagnostic_features)
* [Granted Memory](#anchor_granted_memory)
* [Platform Independence](#anchor_build_requirements)

<a name="anchor_how_to_use_it"></a>
# How to use it

* `$ git clone https://github.com/johsteffens/tbman.git`

<a name="anchor_build_requirements"></a>
## In your workspace

   * Compile `tbman.c` and `btree.c` (either among your source files or into a static library)
   * In your code:
      * `#include "tbman.h"`
      * Call once `tbman_open();` at the beginning or your program. *(E.g. first in `main()`)*
      * Use `tbman_*` - functions anywhere.
      * Call once `tbman_close();` at the end or your program. *(E.g. last in `main()`)*

## C++

In object oriented C++ programming, the direct use of `malloc`, `realloc` or `free` is discouraged in favor of 
using operators `new` and `delete`, which take care of object construction/destruction. 
However, you can overload these operators. This gives you control over the part concerned with memory allocation.

**Example:**

If you add the code below to your program, operators `new` and `delete` will work as intended but use tbman
for allocating and freeing memory.

```C++
void* operator new( size_t size )
{
    return tbman_malloc( size ); 
}

void operator delete( void* p )
{
    tbman_free( p ); 
}
```

<a name="anchor_quick_evaluation"></a>
## Quick Evaluation

`eval.c` is an evaluation program simulating realistic runtime conditions for a memory manager. 
It verifies correct functionality and assesses the processing speed. 
It compares the performance of stdlib functions with tbman functions. 
You can quickly run it yourself.

Enther the folder with source files:
```
$ gcc -std=c11 -O3 btree.c tbman.c eval.c -lm -lpthread
$ ./a.out
```

## Requirements/Dependencies

   * Compiler supporting the C11 standard (e.g. gcc, clang).
   * Compiler options: `-std=c11`, `-O3` for max speed; (or compatible settings)
   * Linker options: `-lm -lpthread` (or compatible settings)
   * **POSIX**: Out of the box, tbman relies on two features, which are normally available on POSIX compliant systems
      * [Flat Memory Model](#anchor_memory_model).
      * Library pthread: Tbman uses `pthread_mutex_t` (locking) for thread safety in `tbman.c`.
      * The following platforms have sufficient POSIX compliance: **Linux, Android, Darwin (and related OS)**

   * **If pthread is not available, try one of the folowing options ...**:
      * Check if the [^Tread Support Library^](https://en.cppreference.com/w/c/thread) is available for your traget platform. In tbman.c: Use **mtx_t** instead of **pthread_mutex_t**; replace functions **pthread_mutex_...** with corresponding functions **mtx_...**; use **call_once** instead of **pthread_once**.

      * If you have other native locks available: In `tbman.c`: Replace pthread-locks by native locks.

      * **Windows**: You can setup a posix subsystem
         * [Set up a POSIX-environment via cygwin.](https://github.com/johsteffens/beth/wiki/Requirements#how-to-setup-a-posix-environment-for-beth-on-windows)
         * Windows 10: Provides an optional Linux-Subsystem.

<a name="anchor_features"></a>
# Detailed Description

<a name="anchor_basic"></a>
## Basics

Tbman offers the three basic functions of a memory manager:

```C
void* tbman_malloc(             size_t size ); // pure allocation
void* tbman_realloc( void* ptr, size_t size ); // reallocation
void  tbman_free(    void* ptr              ); // freeing
```
Usage and behavior is compatible to corresponding stdlib functions `malloc`, `free`, `realloc`.
<br><sub>Exception: Should the entire system run out of available memory,
tbman aborts with an error message to stderr.</sub>

Tbman must be initialized once before usage. It should also be properly closed at the end of the program. 
These two functions take care of it:

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
## Faster collection

If you free or reallocate memory and know the previously allocated amount, you can further speed up processing by 
telling tbman about the currently allocated size using `tbman_nrealloc` and `tbman_nfree`. 
This helps the manager finding the corresponding node for the memory instance faster.

```C
// realloc with size communication
void* tbman_nrealloc( void* current_ptr, size_t current_size, size_t new_size ); 

// free with size communication
void  tbman_nfree( void* current_ptr, size_t current_size );
```
`current_size` must hold either the requested amount or the [granted amount](#anchor_granted_memory)
for the memory instance addressed by `current_ptr`.

<a name="anchor_one_function_for_everything"></a>
## One function for everything

Alternatively, you can use one of the following two functions for memory management including some special features of tbman.

```C
void* tbman_alloc(  void* current_ptr,                      size_t requested_size, size_t* granted_size );
void* tbman_nalloc( void* current_ptr, size_t current_size, size_t requested_size, size_t* granted_size );
```
`tbman_nalloc` works slightly faster than `tbman_alloc` but requires extra size input.
The two functions can also be mixed; even serving the same memory instance.

### Arguments

| Name      | Description  |
| :----     | :---------------------------  |
| **`current_ptr`**    | Pointer to current memory instance for freeing or reallocating. <br> Set to `NULL` for pure allocation. |
| **`current_size`**   | Previously requested or [granted size](#anchor_granted_memory) for freeing or reallocating a memory instance. <br> Set to `0` for pure allocation. |
| **`requested_size`** | Requested new size pure allocation or reallocation.<br> Set to `0` for freeing.|
| **`granted_size`** | Optional pointer to variable where the function stores the [granted amount](#anchor_granted_memory).<br> Set to `NULL` when not needed. |

### Return value

Pointer to new memory instance for pure allocation or reallocation. Returns `NULL` in case of freeing.

<a name="anchor_automatic_alignment"></a>
## Automatic Alignment

Tbman aligns the memory instance selectively. 
This covers all standard C/C++ data types `char, short, int, float, double, etc`
and also larger types such as `int32x4_t, float32x4_t, etc`, which are typically 
used for SIMD-extensions such as `SSE, AVX, NEON, etc`.

**Example:**

```C
int32x4_t* my_data = tbman_malloc( sizeof( int32x4_t ) * 10 ); // aligned array of 10 x int32x4_t
```

<a name="anchor_granted_memory"></a>
## Granted Memory

For design reasons tbman might find no proper use for some space immediately following your requested memory block.
In that case it grants you that extra space, appending it to your request.
You may use the granted space as if you had requested it in the first place.
<br><sub>*(Note: Tbman never grants less than requested.)*</sub>

This feature is a special resource for optimizing speed and memory efficiency
of objects that vary allocation size during lifetime.
It is extensively used in [beth](https://github.com/johsteffens/beth) on dynamic arrays.

Function `tbman_alloc` lets you allocate with the [granted amount communicated](#anchor_one_function_for_everything).
You can retrieve the granted amount for a given instance using function `tbman_granted_space`:

```C
// Allocation with granted amount communicated.
void* tbman_alloc( void* current_ptr, size_t requested_size, size_t* granted_size );

// Explicit query for the granted amount from an already existing memory instance
size_t tbman_granted_space( const void* ptr );
```

**Example:**

```C
size_t requested_space = 5;
size_t granted_space;
char* my_string = tbman_alloc( NULL, requested_space, &granted_space );
// At this point granted_space >= requested_space. Using that extra space is allowed.

// To visualize, we fill the requested and extra space with different characters:
for( size_t i = 0; i < requested_space - 1; i++ ) my_string[ i ] = '=';
for( size_t i = requested_space - 1; i < granted_space - 1; i++ ) my_string[ i ] = '#';
my_string[ granted_space - 1 ] = 0;

// Possible output:
// ====###
printf( "%s\n", my_string );
```

<a name="anchor_diagnostic_features"></a>
## Diagnostic Features

Tbman offers a few features for advanced memory analysis.
They are useful for debugging, ensuring memory integrity or even developing a garbage-collection scheme.

<a name="anchor_integrated_leak_detection"></a>
### Integrated Leak Detection

Functions `tbman_close()` and `tbman_s_close()` check for leaking memory instances.
These are instances allocated but not freed before closing the manager.
If any are found, a message is send to stderr.

**Example:**

```C
tbman_open();

// We are deliberately leaking some memory.
tbman_malloc( 13 );
tbman_malloc( 7 );

// tbman_close() will produce a message like this:
// TBMAN WARNING: Detected 2 instances with a total of 24 bytes leaking space.
tbman_close();
```

<a name="anchor_memory_tracking"></a>
### Memory Tracking

You can query the total of tbman-allocations at any point in your program. The following functions do this:
```C
// returns the current number of allocations (memory instances)
size_t tbman_total_instances( void );

// returns the number of bytes currently allocated
size_t tbman_total_granted_space( void ); 
```
Possible use-cases:

   * Hunting down memory leaks.
   * Assessing the memory footprint of a program, sections thereof or of specific objects.

**Example:**

```C
size_t prior_space = tbman_total_granted_space();
size_t prior_insts = tbman_total_instances();

... // section to be tested for leaks

size_t leaking_space = tbman_total_granted_space() - prior_space;
size_t leaking_insts = tbman_total_instances() - prior_insts;
if( leaking_insts > 0 )
{
    fprintf( stderr, 
        "Memory leak of %zu bytes detected. %zu instances were not freed.\n", 
        leaking_space, 
        leaking_insts );
}
```

<a name="anchor_memory_tracking"></a>
### Iterating through instances

The following function lets you iterate through all instances currently allocated:

```C
void tbman_for_each_instance( 
    void (*cb)( void* arg, void* ptr, size_t space ), 
    void* arg );
```

`cb` is a *callback function*. It is called for each instance active at the time of calling `tbman_for_each_instance`.

#### Arguments

| Name      | Description  |
| :----     | :---------------------------  |
| **`cb`**    | Pointer to callback function |
| **`arg`**   | Custom argument passed to each callback |
| **`ptr`**   | Address of the memory instance |
| **`space`** | Number of bytes granted to the instance |

Changing tbman's state inside a callback function is allowed. E.g. The callback function may free or allocate memory.
Keep in mind, though, that all instances, producing a callback, are determined before executing the first callback.
Freeing or allocating inside a callback will therefore not affect the callback-order.
It will only affect the next call to `tbman_for_each_instance`.

**Example:**

A trivial garbage collector, simply freeing all remaining open instances and counting them.

```C
// frees the instance assuming arg references a counter
void free_instance_callback( void* arg, void* ptr, size_t space )
{
    tbman_nalloc( ptr, space, 0, NULL );
    if( arg ) (*(size_t*)arg)++;
} 
```

```C
... // somewhere in code
size_t count = 0;
tbman_for_each_instance( free_instance_callback, &count );
printf( "%zu instances were freed.\n", count );

```

<a name="anchor_thread_safety"></a>
## Thread safety

Tbman is thread safe: The interface functions can be called any time from any thread simultaneously.
Memory allocated in one thread can be freed in any other thread.

Concurrency is governed by a mutex.
This means that memory management is not lock-free.
Normally, this will not significantly affect processing speed for typical multi threaded programs.
Only during heavvy simultaneous usage of the same manager lock-contention time might be noticeable
compared to single threaded usage.

<a name="anchor_multiple_managers"></a>
## Multiple managers

Functions `tbman_` above relate to global management (one manager for everything).
You can also create multiple individual, independent and dedicated managers using the the `tbman_s` object.
Each manager has its own mutex.
This is particularly helpful in a multi threaded context.
Giving each thread its own manager for thread-local memory can reduce lock-contention.

For each of above functions `tbman_` there exists a corresponding function with postfix `_s`
meant for a dedicated manager instance.
Except `tbman_s_open`, all functions `tbman_s_` take as first argument the reference to the dedicated manager instance.

**Example:**

```C 
tbman_s* my_man = tbman_s_open(); // opens a dedicated manager
char* my_memory = tbman_s_malloc( my_man, 1024 );
... // do something else
tbman_s_free( my_man, my_memory );  
tbman_s_close( my_man ); // closes a dedicated manager
```
<a name="anchor_mixing_different_memory_managers"></a>
## Mixing different memory managers

Tbman does not affect the behavior of other memory managers (such as `malloc`, `free`, `realloc`),
so you can mix code using different management systems.

However, different managers can not serve the **same memory instance**.
For example: You can not use `free` or `realloc` on a memory instance,
which was allocated with `tbman_malloc` (`tbman_realloc`) or vice versa.

Likewise, you can not manage the same memory instance with different [dedicated managers](#anchor_multiple_managers).

<a name="anchor_side_effects"></a>
# Side effects

Below are some side effects you should be aware of.
We believe they are tolerable for the vast majority of use cases.

<a name="anchor_prefetching"></a>
## Prefetching

Tbman reserves and returns system memory in larger chunks to offload the system.
That means that the memory your application reserves at a given time is likely
higher than if you use system functions directly.

<a name="anchor_unused_memory"></a>
## Unused Memory

Tbman organizes memory instances into slots with a predefined size distribution.
For an allocation request, the best fitting slot is selected and the full slot-size is [granted](#anchor_granted_memory).
If you do not need that extra amount, it is wasted.
Tests have shown that in realistic situations this overhead tends to average around 10% ... 30% of the requested memory.

*Note that also other memory managers reserve excess memory and/or render memory sections
temporarily unusable (e.g. due to fragmentation).
Which manager is most efficient depends on the use case.*

<a name="anchor_memory_model"></a>
## Memory Model

Tbman expects a flat memory model. More specifically, it requires the following behavior:

If two pointers `ptr1`, `ptr2` reference valid but **different** objects anywhere in the application's addressable
memory space, then `( ptrdiff_t )( ptr1 - ptr2 )` can never be zero.

Although this sounds like a no-brainer, it actually goes beyond the standard C provisions.
Std. C allows the compiler implementation to leave the result of pointer subtraction undefined if the objects are
not of the same array- or structure-instance.
(see [cppreference.com: Pointer arithmetic](https://en.cppreference.com/w/c/language/operator_arithmetic#Pointer_arithmetic).)

*Note that most modern platforms employ a flat memory model.
Very old systems, like early x86 platforms, use a segmented memory model
(segment:offset) where only the offset participates in pointer arithmetic.
On that model tbman would not work correctly.*

<a name="anchor_debugging_tools"></a>
## Debugging Tools

Certain debugging tools (e.g. [valgrind](http://www.valgrind.org)) can analyze the memory integrity of a program.
One of the methods employed is capturing interactions of the program with the system.

Since tbman represents an intermediate layer between your program and the system,
effectively reducing system interactions, the tool captures less activity. For example,
it might not recognize all the boundaries of a single tbman-allocation and can therefore
not verify the validity of all types of block-access by the program.
Since `tbman_close()` returns all [tbman-pools](#anchor_block-pooling-layer) to the system,
the tool might also not detect all possible memory leaks.

Function `tbman_close` [checks for leaks](#anchor_integrated_leak_detection) and reports them to stderr,
which should alleviate the last side effect for most practical purposes.

You can can also create a custom-check for leaks in your program by testing
`tbman_total_granted_space()` before closing tbman:

**Example:**

```C 
int main( int argc, char* argv[] )
{
   tbman_open();
   
   ... // my program
   
   if( tbman_total_granted_space() > 0 )
   {
      fprintf( stderr, "Memory leak of %zu bytes detected.\n", tbman_total_granted_space() );
   }
   tbman_close();
   return my_exit_state;
}
```

<a name="anchor_how_it_works_internally"></a>
# How it works internally

<a name="anchor_block-pooling-layer"></a>
## Block-Pooling-Layer with Tokens

Tbman represents a dedicated management layer, which sits between your code and the system. It communicates with the system to obtain/return larger memory blocks, which are subdivided for dispatching/recollection in your program.

Tbman uses "conservative" memory pooling with multiple fixed size block-managers at a strategic size-distribution.
Multiple pools are managed in a btree.
When the client (your code) requests or returns small-medium sized memory instances,
tbman dispatches/recollects pool memory accordingly without initiating system requests.
System requests are executed infrequently in order to acquire a new pool or return an empty pool.
This offloads the system manager significantly.
Compared to always using system calls it can speed up overall processing and/or reduce fragmentation,
particularly in programs where many small sized memory instances are used.

Each memory instance is associated with an internal node controlled by tbman.
The manager dedicates separate memory areas for node-data and user space (user space == memory space used by the client).
The content of user space does not affect node management.

A special design feature is binding the allocated memory address to the address of the associated node in tbman.
This allows fast node retrieval at O(1) complexity. Since the association is address-based (and not meta-data-based
as typical in some conventional memory managers),
it cannot be altered or destroyed by a faulty memory override in user space. Hence, specific software bugs such as using a 
dangling pointer (pointer to already collected memory) or writing past allocated space, are less likely to affect manager's
integrity. (Such bugs still affect the integity of your program, though.).

The design ensures very low latency for allocation and collection and it gives this manager its name:
tbman = token-block-manager.

When the client requests a large memory instance, where pooling would be wasteful,
tbman falls back to using a direct system call.
However, it [keeps track](#anchor_memory_tracking) of all memory.

## Memory alignment

Tbman analyzes the requested size.
If you allocate an instance or array of type `my_type` with `sizeof( my_type )` being a power of two not larger than
[`TBMAN_ALIGN`](https://github.com/johsteffens/tbman/blob/848bebed1648d66d1fe101ee19f4803fed8ea81a/tbman.c#L43),
then the memory block is alinged to `sizeof( my_type )`.
More generally: When requesting memory of _**s**_ bytes and _**s**_ can be expressed as product of two positive
integers _**s**_ = _**m**\***n**_ such that _**m**_ is a power of 2,
then the returned memory is aligned to the lesser of _**m**_ and
[`TBMAN_ALIGN`](https://github.com/johsteffens/tbman/blob/848bebed1648d66d1fe101ee19f4803fed8ea81a/tbman.c#L43).

<a name="anchor_motivation"></a>
# Motivation / Origin

Tbman has originally been conceived and developed for the project
[beth](https://github.com/johsteffens/beth).

For those interested in elementary memory management
but not keen on digesting the whole of project beth, 
we offer herewith a simplified and better documented spin-off.

The beth-memory-manager provides additionally: 

   * Integrated Reference Management
   * Garbage Collection 
   * ... and more

Location: [beth/lib/bcore/](https://github.com/johsteffens/beth/tree/master/lib/bcore)bcore_tbman.*
*(Note that [beth](https://github.com/johsteffens/beth) carries a different
[license](https://github.com/johsteffens/beth/blob/master/LICENSE).)*

------

Thanks for reading. If you find it useful, have questions or suggestions I'd be happy to hear from you.

<sub>&copy; Johannes B. Steffens</sub>
