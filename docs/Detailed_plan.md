# Detailed Plan — Step-by-Step Code

A hands-on companion to `PLAN.md`. This file gives you **complete, working code
with line-by-line explanation** for the next few phases, so you can build them
offline without needing to ask. Follow the parts in order.

Everything here matches the conventions you already use:
- `Guest` struct lives in `src/guest.h` (`int guest_number; char name[NAME_MAX];`, with `#define NAME_MAX 64`).
- You compile with `-Isrc`, so includes are written relative to `src/`
  (e.g. `#include "guest.h"`, `#include "hashmaps/chaining.h"`).
- Each technique = one `.c` + one `.h` in `src/hashmaps/`.
- Hash functions go in `src/hashfuncs/`.
- Tests go in `tests/`, each with its own `main()`, compiled separately.

**Table of contents**
- [Part A — Phase 1.5: Naive modulo hashmap](#part-a)
- [Part B — String hash functions (sum, djb2, FNV-1a)](#part-b)
- [Part C — Phase 2: Separate chaining](#part-c)
- [Part D — Compile command reference](#part-d)
- [Part E — New C concepts glossary](#part-e)

---

<a name="part-a"></a>
## Part A — Phase 1.5: Naive Modulo Hashmap

**Goal:** a hash table with **one guest per slot** and **no collision handling** —
a second guest hashing to an occupied slot silently *overwrites* the first.
This is the deliberately-broken baseline that motivates chaining.

The index rule is `guest_number % capacity` instead of direct addressing's
`guest_number - 1`. With capacity 1000 and guests 1–1000, everyone gets a unique
slot (it looks like it works!). Guest **1001** is where it breaks:
`1001 % 1000 = 1`, colliding with guest 1 — who gets erased.

### A1. The header — `src/hashmaps/naive_hash.h`

```c
#ifndef NAIVE_HASH_H
#define NAIVE_HASH_H

#include "guest.h"

#define NAIVE_CAPACITY 1000

void naive_insert(int guest_number, const char *name);
Guest *naive_lookup(int guest_number);
int naive_collision_count(void);

#endif
```

- `#include "guest.h"` — pulls in the `Guest` struct and `NAME_MAX`.
- `NAIVE_CAPACITY 1000` — the table size. (Try `100` later for a more dramatic
  demo where collisions happen even below 1000 guests.)
- `naive_collision_count` — returns how many overwrites happened, so `main` can
  report the data loss.

### A2. The implementation — `src/hashmaps/naive_hash.c`

```c
#include "hashmaps/naive_hash.h"
#include <string.h>

// One guest per slot. File-scope array → auto-zeroed at startup,
// so every slot's guest_number starts at 0 (our "empty" sentinel).
static Guest table[NAIVE_CAPACITY];
static int collisions = 0;

void naive_insert(int guest_number, const char *name) {
    int index = guest_number % NAIVE_CAPACITY;

    // Collision = slot already holds a DIFFERENT guest. Count it, then overwrite.
    if (table[index].guest_number != 0 &&
        table[index].guest_number != guest_number) {
        collisions++;
    }

    table[index].guest_number = guest_number;
    strncpy(table[index].name, name, NAME_MAX - 1);
    table[index].name[NAME_MAX - 1] = '\0';
}

Guest *naive_lookup(int guest_number) {
    int index = guest_number % NAIVE_CAPACITY;

    // If the slot doesn't hold THIS guest, they're not here
    // (either empty, or someone overwrote them).
    if (table[index].guest_number != guest_number) {
        return NULL;
    }
    return &table[index];
}

int naive_collision_count(void) {
    return collisions;
}
```

Line-by-line:
- `int index = guest_number % NAIVE_CAPACITY;` — the modulo keeps the index in
  range `0..999` no matter how big `guest_number` gets. This is the one change
  from direct addressing.
- The collision `if` — a slot is "occupied by someone else" when its stored
  `guest_number` is neither 0 (empty) nor the same guest we're inserting.
- The `strncpy` + manual `'\0'` — same safe-copy idiom as direct addressing.
- In `naive_lookup`, `table[index].guest_number != guest_number` is the key
  check: after an overwrite, the slot holds the *new* guest's number, so a
  lookup for the *old* guest correctly returns `NULL` — they're gone.

### A3. The demo in `main.c`

After you load all 1000 guests (your existing file-reading loop, but calling
`naive_insert` instead of `insert_guest`), add this:

```c
    // Confirm guest 1 is present before the collision
    Guest *before = naive_lookup(1);
    printf("Before: guest 1 = %s\n", before ? before->name : "(missing)");

    // A new guest checks in — 1001 % 1000 = 1, collides with guest 1
    naive_insert(1001, "New Arrival");

    // Guest 1 has been silently overwritten
    Guest *after = naive_lookup(1);
    printf("After:  guest 1 = %s\n", after ? after->name : "(missing)");
    printf("Guest 1001 = %s\n", naive_lookup(1001)->name);
    printf("Total collisions (overwrites): %d\n", naive_collision_count());
```

Expected output:
```
Before: guest 1 = Pamela James
After:  guest 1 = (missing)
Guest 1001 = New Arrival
Total collisions (overwrites): 1
```

`before ? before->name : "(missing)"` is a **ternary** — "if `before` is not
NULL, use `before->name`, else use the string `(missing)`." Compact if/else.

### A4. Compile & run

```bash
gcc -Wall -Wextra -Isrc -o main main.c src/hashmaps/*.c
./main
```

That "(missing)" line is the whole lesson: **the naive hashmap loses data on
collision.** That's what chaining fixes.

---

<a name="part-b"></a>
## Part B — String Hash Functions

Chaining keys by `name` (a string), so you need a function that turns a string
into a number. Build three: one deliberately bad, two good — so you can later
compare how collision-prone each is on the same hashmap.

All three share one signature, so they're interchangeable:
```c
unsigned long some_hash(const char *key);
```

### B1. The header — `src/hashfuncs/string_hashes.h`

```c
#ifndef STRING_HASHES_H
#define STRING_HASHES_H

unsigned long hash_sum(const char *key);    // naive: sum of characters (bad)
unsigned long hash_djb2(const char *key);   // djb2 (good)
unsigned long hash_fnv1a(const char *key);  // FNV-1a (good)

#endif
```

### B2. The implementation — `src/hashfuncs/string_hashes.c`

```c
#include "hashfuncs/string_hashes.h"

// NAIVE: sum of character codes. Deliberately bad — order doesn't matter,
// so "abc", "cab", "bca" all collide. Good for showing what a BAD hash does.
unsigned long hash_sum(const char *key) {
    unsigned long hash = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        hash += (unsigned char) key[i];
    }
    return hash;
}

// djb2: hash = hash * 33 + c, folded one char at a time.
// Position-sensitive (a rolling polynomial), so anagrams DON'T collide.
unsigned long hash_djb2(const char *key) {
    unsigned long hash = 5381;               // the magic starting value
    for (int i = 0; key[i] != '\0'; i++) {
        hash = hash * 33 + (unsigned char) key[i];
    }
    return hash;
}

// FNV-1a: XOR the char in, then multiply by a big prime. Also position-sensitive.
unsigned long hash_fnv1a(const char *key) {
    unsigned long hash = 14695981039346656037UL;  // FNV offset basis (64-bit)
    for (int i = 0; key[i] != '\0'; i++) {
        hash ^= (unsigned char) key[i];            // XOR this char in
        hash *= 1099511628211UL;                   // multiply by FNV prime
    }
    return hash;
}
```

Key points:
- **`key[i] != '\0'`** — loops until the string's null terminator, i.e. over
  every real character.
- **`(unsigned char) key[i]`** — `char` can be signed, so a byte over 127 would
  come out negative and mess up the math. Casting to `unsigned char` keeps it
  0–255. Do this in every string hash.
- **`unsigned long`** — the hash is *supposed* to overflow and wrap around;
  unsigned overflow is well-defined in C (signed overflow is undefined
  behavior — a real bug). Always use unsigned for hash accumulators.
- **Why `hash_sum` is bad:** it only *adds*, so character order is lost —
  "Anna Smith" and "Nana Smith" (same letters) collide. `djb2`/`fnv1a` multiply
  as they go, so position matters and near-identical names scatter apart.
- **To get a table index:** `hash_djb2(name) % capacity`.

---

<a name="part-c"></a>
## Part C — Phase 2: Separate Chaining

**Goal:** each bucket holds a *linked list* of guests. Collisions add a node to
the list instead of overwriting. No data loss. This is the real fix.

This introduces the biggest new C concepts: **linked lists**, **`malloc`/`free`**
(heap memory), and **function pointers** (injecting the hash function). Read
Part E's glossary alongside this.

### C1. The header — `src/hashmaps/chaining.h`

```c
#ifndef CHAINING_H
#define CHAINING_H

#include "guest.h"

// One node in a bucket's linked list: a guest + a pointer to the next node.
typedef struct Node {
    Guest guest;
    struct Node *next;     // NULL marks the end of the chain
} Node;

// The chaining hash table.
typedef struct {
    Node **buckets;                          // array of list-heads (pointers)
    int capacity;
    unsigned long (*hash)(const char *key);  // injected hash function
} ChainMap;

ChainMap *chain_create(int capacity, unsigned long (*hash)(const char *key));
void chain_insert(ChainMap *map, int guest_number, const char *name);
Guest *chain_lookup(ChainMap *map, const char *name);
void chain_destroy(ChainMap *map);

#endif
```

Understanding the two structs:
- **`Node`** — holds one `Guest` and a `next` pointer to another `Node`.
  A struct containing a pointer *to its own type* is what makes a chain: node →
  node → node → NULL. Note `struct Node *next` uses the full `struct Node`
  name, because the `Node` typedef alias isn't finished being defined yet on
  that line.
- **`ChainMap`** — `buckets` is a `Node **` (a pointer to an array of `Node *`).
  Each element is the head of one bucket's chain (or NULL if empty). `hash` is a
  **function pointer** — you pass in `hash_djb2` (or any string hash) at create
  time, so one chaining implementation works with any hash function.

### C2. The implementation — `src/hashmaps/chaining.c`

```c
#include "hashmaps/chaining.h"
#include <stdlib.h>   // malloc, calloc, free
#include <string.h>   // strncpy, strcmp

// Allocate a new, empty chaining map on the heap.
ChainMap *chain_create(int capacity, unsigned long (*hash)(const char *key)) {
    ChainMap *map = malloc(sizeof(ChainMap));
    map->capacity = capacity;
    map->hash = hash;
    // calloc zeroes the memory, so every bucket starts as NULL (an empty chain).
    map->buckets = calloc(capacity, sizeof(Node *));
    return map;
}

// Add a guest. New nodes go at the FRONT of the bucket's chain (O(1)).
void chain_insert(ChainMap *map, int guest_number, const char *name) {
    unsigned long index = map->hash(name) % map->capacity;

    // 1. Make a new node on the heap and fill it in.
    Node *node = malloc(sizeof(Node));
    node->guest.guest_number = guest_number;
    strncpy(node->guest.name, name, NAME_MAX - 1);
    node->guest.name[NAME_MAX - 1] = '\0';

    // 2. Point the new node at the current head, then make it the new head.
    node->next = map->buckets[index];
    map->buckets[index] = node;
}

// Find a guest by name: hash to a bucket, then walk that bucket's chain.
Guest *chain_lookup(ChainMap *map, const char *name) {
    unsigned long index = map->hash(name) % map->capacity;

    Node *current = map->buckets[index];
    while (current != NULL) {
        if (strcmp(current->guest.name, name) == 0) {
            return &current->guest;      // found it
        }
        current = current->next;         // move to next node in the chain
    }
    return NULL;                         // walked the whole chain, not here
}

// Free EVERYTHING: every node, the bucket array, then the map struct.
void chain_destroy(ChainMap *map) {
    for (int i = 0; i < map->capacity; i++) {
        Node *current = map->buckets[i];
        while (current != NULL) {
            Node *next = current->next;  // save next BEFORE freeing current
            free(current);
            current = next;
        }
    }
    free(map->buckets);
    free(map);
}
```

Line-by-line on the new/tricky parts:

**`chain_create`:**
- `malloc(sizeof(ChainMap))` — asks the OS for a chunk of heap memory big enough
  for one `ChainMap`, returns a pointer to it. Unlike a local variable, heap
  memory lives until you `free` it.
- `calloc(capacity, sizeof(Node *))` — allocates room for `capacity` pointers
  AND zeroes them. Zero for a pointer means NULL, so every bucket correctly
  starts as an empty chain. (`malloc` does *not* zero — that's why we use
  `calloc` here.)

**`chain_insert` — front insertion (the clever bit):**
- `node->next = map->buckets[index];` — point the new node at whatever the
  bucket currently holds (the old head, or NULL if empty).
- `map->buckets[index] = node;` — make the new node the head.
- Two lines, always O(1), no walking the list. Order in the chain ends up
  reverse-insertion, which doesn't matter for a hashmap.

**`chain_lookup` — walking the chain:**
- Start at the bucket's head, follow `next` pointers until you either match the
  name (`strcmp(...) == 0`) or hit NULL (end of chain).
- You only ever scan *one bucket's chain*, never the whole table — that's why
  it's fast when chains are short.

**`chain_destroy` — why the `next` dance:**
- `Node *next = current->next;` saves the next pointer *before* `free(current)`,
  because after freeing, `current->next` is invalid memory you can't read.
  Forgetting this is a classic use-after-free bug.
- You must free every node, then the bucket array, then the map itself — three
  levels of allocation, freed inside-out.

> **Production note:** real code checks whether `malloc`/`calloc` returned NULL
> (out of memory) before using the pointer. Omitted here for readability; add
> `if (node == NULL) { /* handle */ }` if you want to be thorough.

### C3. Using it (in `main.c` or a scratch file)

```c
#include "hashmaps/chaining.h"
#include "hashfuncs/string_hashes.h"

// ... inside main, after including the above ...

// Create a chaining map with 256 buckets, using djb2 as the hash function.
ChainMap *map = chain_create(256, hash_djb2);

// In your file-reading loop, instead of naive_insert:
chain_insert(map, guest_number, name);

// Look someone up by NAME (chaining keys by name, not number):
Guest *g = chain_lookup(map, "Pamela James");
if (g != NULL) {
    printf("Found %s (guest #%d)\n", g->name, g->guest_number);
} else {
    printf("Not found\n");
}

// When done, free everything:
chain_destroy(map);
```

Swap `hash_djb2` for `hash_sum` or `hash_fnv1a` to compare hash functions on the
exact same map — that's the whole point of injecting the hash as a parameter.

Note: the dataset has 30 duplicate names on purpose. With chaining, both guests
with the same name end up in the same chain; `chain_lookup` returns the first
one it finds (the most recently inserted). That's expected, not a bug.

### C4. A test — `tests/test_chaining.c`

```c
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hashmaps/chaining.h"
#include "hashfuncs/string_hashes.h"

int main(void) {
    ChainMap *map = chain_create(16, hash_djb2);

    chain_insert(map, 1, "Pamela James");
    chain_insert(map, 2, "Debra Thompson");

    Guest *g = chain_lookup(map, "Pamela James");
    assert(g != NULL);
    assert(g->guest_number == 1);
    assert(strcmp(g->name, "Pamela James") == 0);

    Guest *missing = chain_lookup(map, "Nobody Here");
    assert(missing == NULL);

    chain_destroy(map);
    printf("All chaining tests passed.\n");
    return 0;
}
```

Compile the test (note it needs the chaining AND hashfuncs source files):
```bash
gcc -Wall -Wextra -Isrc -o test_chaining \
    tests/test_chaining.c src/hashmaps/chaining.c src/hashfuncs/string_hashes.c
./test_chaining
```

---

<a name="part-d"></a>
## Part D — Compile Command Reference

Once you have hash functions in `src/hashfuncs/`, the main build grows to include
that folder too:

```bash
# Full program (all hashmaps + all hash functions)
gcc -Wall -Wextra -Isrc -o main main.c src/hashmaps/*.c src/hashfuncs/*.c
./main
```

```bash
# A single test (list only the files that test needs)
gcc -Wall -Wextra -Isrc -o test_chaining \
    tests/test_chaining.c src/hashmaps/chaining.c src/hashfuncs/string_hashes.c
./test_chaining
```

Reminders:
- `-Isrc` lets includes be written relative to `src/`.
- `*.c` = files directly in that folder (one star = one level). You flattened
  `src/hashmaps/`, so one star is correct.
- Never list `.h` files or two `main()` files together.
- Run `./main` from the project root (it reads `data/hotel_california.psv`).

---

<a name="part-e"></a>
## Part E — New C Concepts Glossary

**`malloc(size)`** — requests `size` bytes of *heap* memory, returns a pointer to
it (or NULL if out of memory). Heap memory lives until you `free` it — unlike
local variables, which vanish when their function returns. Use `sizeof(Type)` to
get the right size: `malloc(sizeof(Node))`.

**`calloc(count, size)`** — like `malloc` for an *array* of `count` items, and it
**zeroes** the memory. Use it when zero is a meaningful starting state — e.g. an
array of pointers you want to all start NULL.

**`free(ptr)`** — returns heap memory to the system. Every `malloc`/`calloc`
must eventually be `free`d, or you leak memory. Never use a pointer after
freeing it (use-after-free), and never free the same pointer twice (double-free).

**Function pointer** — a variable holding *which function to call*. Type
`unsigned long (*hash)(const char *key)` means "a pointer to a function taking a
`const char *` and returning `unsigned long`." You call it like a normal
function: `map->hash(name)`. This is how one hashmap works with any hash function.

**Linked list** — a chain of nodes, each holding data + a `next` pointer to the
following node. The last node's `next` is NULL. You traverse it with
`while (current != NULL) { ...; current = current->next; }`.

**`->` vs `.`** — `.` accesses a field of a struct you *have*
(`node.guest`); `->` accesses a field through a *pointer* to a struct
(`current->guest`). `current->guest` is shorthand for `(*current).guest`.

**`typedef struct { ... } Name;`** — defines a struct and gives it a short alias
`Name` so you can write `Name x;` instead of `struct Name x;`. For a
self-referential struct (a node pointing to its own type) you must give the
struct a tag — `typedef struct Node { ... struct Node *next; } Node;` — because
the `Node` alias isn't usable yet on the line that references it.

**Ternary `cond ? a : b`** — a compact if/else that produces a value: "if `cond`
is true, use `a`, else `b`." Example: `g ? g->name : "(missing)"`.
