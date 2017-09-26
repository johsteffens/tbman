# Tbman - Fast and easy-to-use general-purpose Memory Manager

## What it is
Tbman is a memory manager offering (among others) these functions 

`tbman_malloc, tbman_free, tbman_realloc`

which can replace corresponding stdlib's functions

`malloc, free, realloc`

in C and C++ programs.

## Benefits
* Generally faster than stdlib functions. [*You can quickly verify this yourself.*](#how-to-use-it)
* Automatic alignment even for extended data types (E.g. SIMD types such as int32x4_t)
* Reduces fragmentation of system memory.
* Additional features:
    * Easy monitoring of total memory usage (e.g. for leak detection)
    * Optional communicaton of actually granted amounts (useful for dynamic arrays)

## How to use it
**Quick evaluation**:
* `gcc -std=c11 -O3 *.c -lm -lpthread; ./a.out`

**In your code:**
* Compile tbman.c and btree.c (either among your source files or into a static library)
* In your code: `#include "tbman.h"`
* At the beginning or your program call once: `tbman_open();`
* Use other tbman-functions.
* At the end or your program call once: `tbman_close();`

**Build Requirements:**
* `-std=c11`
* `-lpthread`  (tbman uses pthread_mutex for thread safety)

## How it works
Tbman uses a "conservative" memory pooling approach with multiple token-based fixed size block-managers at a strategic size-distribution. It pre-allocates only moderate amounts of memory and dymatically acquires more or releases back to the system as needed and/or suitable. Multiple pools are managed in a btree.

For large memory requests, where pooling would be wasteful, tbman falls back to using direct system calls. However, it keeps track of all granted memory.

**More Details:**
* *Coming soon ...*

## Motivation
This memory manager has been developed for the project [beth](https://github.com/johsteffens/beth). It reached sufficient maturity for general purpose and platform independent usage. We therefore created the spin-off-project [tbman](https://github.com/johsteffens/tbman) where the manager is offered as compact stand-alone solution.

I hope you will find it useful.

<sub>*Johannes Steffens*</sub>
