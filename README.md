# Tbman - Fast and easy-to-use general-purpose Memory Manager

## What it is
Tbman is a memory manager offering (among others) these functions 
* tbman_malloc, 
* tbman_free 
* tbman_realloc 

which can replace corresponding stdlib's functions

* malloc
* free
* realloc 

in C and C++ programs.

## Benefits
* Generally faster than stdlib functions. [*You can quickly verify this yourself.*](#how-to-use-it)
* Monitoring total memory usage of your program.
* Additional features to further improve memory control and efficiency in your programs.
* Extended automatic data alignment (E.g. SIMD data types such as int32_4_t)

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
Tbman employs memory pooling via token-based fixed size block-managers.
Multiple such pools are strategically combined and managed in a btree.
It uses/falls-back-to system functions for acquiring memory pools, for its own memory needs and for large memory requests.

**More Details:**
* *Coming soon ...*

## Motivation for this project
This memory manager has been developed for the project [beth](https://github.com/johsteffens/beth). It reached sufficient maturity for general purpose and platform independent usage. We therefore created this spin-off-project [Tbman](https://github.com/johsteffens/tbman) where the manager is offered as compact stand-alone solution.

I hope you will find it useful.

<sub>*Johannes Steffens*</sub>
