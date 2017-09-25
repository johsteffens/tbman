# Tbman - Fast and easy to use general purpose Memory Manager

## What it is
Tbman is a memory manager offering functions 
* tbman_malloc, 
* tbman_free 
* tbman_realloc 

which can replace corresponding stdlib's functions

* malloc
* free
* realloc 

in C and C++ programs to speed up memory management in your programs and to enrich your project with additional features.

## Benefits
* Generally faster than stdlib functions. *You can quickly verify this yourself.*
* Monitoring total memory usage of your program.
* Additional alloc-features to further improve memory efficiency in your programs.

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

## Motivation for this project
The management algorithm has been conceived and designed for the project [beth](https://github.com/johsteffens/beth).
[Tbman](https://github.com/johsteffens/tbman) is a stand-alone spin-off offered for general purpose usage. 
I hope it will speed up your program and the you find the additional features useful.

**More Details:**
* *Coming soon ...*
