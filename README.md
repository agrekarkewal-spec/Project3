# AVL-Backed Set ADT (C)

A Set data structure built on top of a self-balancing AVL tree, written in
plain C with no external libraries.

## Why an AVL tree for a set?

A set just needs to store unique values and let you check membership
quickly. An AVL tree gives you that almost for free — since it rejects
duplicate keys on insert, "no duplicates allowed" comes built in, and
because it stays balanced, insert/delete/search all run in O(log n) even
in the worst case (unlike a plain BST, which can degrade into a linked
list on sorted input).

## What's in here

```
avl_set.h   The public API — everything you can call from outside
avl_set.c   The actual implementation (AVL tree + set logic)
main.c      A small interactive program that lets you try it out
```

`avl_set.h` is the only file you need to read to use the library. The
`AVLSet` type is opaque (you never see its internal fields), so the tree
can't be accidentally corrupted from outside — you always go through the
provided functions.

## What it can do

- Basic set stuff: insert, remove, check membership, get the size
- Set math: union, intersection, difference, symmetric difference,
  subset check, equality check — each one returns a brand new set and
  leaves the originals untouched
- Save a set to a file and load it back later
- Export the result of an operation (like a union) to a file, with a
  label, so you know what it was
- Import several saved sets at once

## Building it

No Makefile needed, just one line:

```bash
gcc -Wall -Wextra -std=c11 avl_set.c main.c -o avl_set_demo
./avl_set_demo
```

## A quick taste of the API

```c
#include "avl_set.h"

AVLSet *a = avlset_create();
AVLSet *b = avlset_create();

avlset_insert(a, 10, NULL);
avlset_insert(a, 20, NULL);
avlset_insert(b, 20, NULL);
avlset_insert(b, 30, NULL);

AVLSet *result = avlset_union(a, b);   // { 10, 20, 30 }

avlset_destroy(a);
avlset_destroy(b);
avlset_destroy(result);
```

Every function that hands you a set back is documented in `avl_set.h`
along with which function you should use to free it — worth a skim before
you dive in.

## File format

Saved/exported sets look like this — plain and readable, nothing fancy:

```
# AVLSET v1
# label: A union B
# count: 3
10
20
30
```

## Ideas for taking it further

- Make it generic (store more than just `int`, via a comparator function)
- Add a benchmark comparing it against a naive array-based set as data
  grows, to actually show the speed difference
- A multiset variant that tracks how many times each element appears
