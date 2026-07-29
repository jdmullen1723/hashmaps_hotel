# Hashmaps in C — Project Plan

A working plan for this repo, written for both of us to come back to. This is the
source of truth for what we've decided and what's left — update it as decisions
change rather than letting it go stale.

## 1. Vision

Priority order, from the goals discussion:

1. **Learn hashmaps** (top priority) — building arrays in C, traversing them,
   writing hash functions, thinking about data storage conceptually enough to
   invent your own variants.
2. **Learn C** generally.
3. **Portfolio value** — something that reads as substantive to a software/ML
   internship reviewer.
4. **Educational for you later, and possibly others** — you like explaining
   things well; this repo should teach, not just work.

**Decision: CLI-first, README-as-the-lesson.** All effort goes into the C core.
A web layer is explicitly out of scope for v1 (see §11). The reasoning: a solid
C core + a README with real benchmark data is a strong, differentiated portfolio
piece on its own, and every hour spent on presentation polish is an hour not
spent on the C/hashmap fundamentals that are the actual point.

**Priority update (supersedes earlier "engaging terminal experience" framing):**
the CLI is NOT an educational/visual lesson flow. `main`'s job is simply to
**print a spec sheet** — a plain table of benchmark metrics, one row per
(hashmap × hash-function) combination. No animations, no ANSI color, no
narrated walkthrough, no per-insert visuals. The teaching lives entirely in the
README; the CLI is a measurement tool that emits comparable numbers. Any
"engaging visual CLI" work is dropped from v1 (see §8).

## 2. Dataset

**File:** `data/hotel_california.psv` — pipe-delimited, 1,000 guest rows + 1 header row.

**Story — "Hotel California":** a hotel front desk. The hash function *is* the
front desk — it takes a guest and computes their room (slot). Insert = check-in,
lookup = find a guest, delete = check-out. A collision = two guests sent to the
same room. This framing makes every hashmap concept physical: probing = walk
down the hall to the next room; chaining = add a rollaway bed to an occupied
room; tombstone = a "checked out, keep looking" marker; resizing = the hotel
can't just add wings for free.

**Source:** fabricated by us with a seeded generator (reproducible). This is
synthetic data purpose-built to exercise the hashmap operations — *not* scraped.
(An earlier version of this project used the real Titanic passenger dataset; we
pivoted to a hotel because check-in/check-out gives a natural reason for
deletion, tombstones, and grow/shrink resizing. An insert-only historical
dataset like Titanic can't motivate those operations.)

**Schema:**

```
guest_number|name|phone
```

| Column | Type | Notes |
|---|---|---|
| `guest_number` | int | 1–1000, arrival order. Dense sequential key — the ONLY column that makes direct addressing possible. Synthetic. |
| `name` | string | fabricated from common first/last name pools. The string-key candidate (Phase 2 hashing). |
| `phone` | string | 10-digit, no punctuation. A large, **sparse** numeric key — key range ~10^10 vs. 1000 entries, so it **cannot** be direct-addressed. Motivates real numeric hashing. |

**Key decisions made:**

- **`guest_number` is the direct-addressing key**, dense 1–1000. Works only
  because it's dense *and* the guest count is known up front — both assumptions
  break for a real, still-operating hotel (that failure is the Phase 1
  "why it breaks" lesson, and the bridge to real hashing).
- **`name` contains 30 deliberate duplicate entries** (e.g. two different
  "Anna Thompson"s) reintroduced on purpose, so "name alone isn't always a
  unique key" is a real, testable case rather than contrived.
- **`phone` is deliberately sparse.** Chosen over a credit-card number (which
  would look like a data-breach dump in a public GitHub repo) but keeps the
  same large-key-space property that makes direct addressing impossible and
  hashing necessary.
- **Pipe-delimited, not JSON** — avoids needing a real CSV/JSON parser in C
  before any hashmap code exists. Verified no field contains a literal `|`.

## 3. Vocabulary

Canonical definitions — use these consistently in code comments, README, and
the CLI's own output text.

- **Key** — the lookup input (e.g. `guest_number`, `name`, `phone`).
- **Value** — the data associated with the key (the rest of the guest record).
- **Entry** — the stored `{key, value}` unit.
- **Slot** — one location in the backing array (open addressing: holds one entry).
- **Bucket** — one location in chaining (holds a list of entries).
- **Hash function** — maps a key to an integer.
- **Collision** — two different keys mapping to the same slot/bucket.
- **Load factor** — entries / capacity.
- **Capacity** — total slots, independent of how many are filled.
- **Chain** — chaining's per-bucket linked list.
- **Probe sequence** — open addressing's ordered list of slots to try on collision.
- **Tombstone** — a deletion marker in open addressing so lookups don't stop early.
- **Map vs. multimap** — one value per key vs. a list of values per key.

## 4. Architecture

**Guest struct** — one shared guest record type in `src/guest.h`, used by
every hashmap implementation regardless of collision strategy. (Currently holds
`guest_number` + `name`; `phone` gets added when a phase needs it.)

**Hash function interface** — two signatures, since numeric and string keys
are genuinely different inputs:

```c
typedef unsigned long (*int_hash_fn_t)(long key);
typedef unsigned long (*str_hash_fn_t)(const char *key);
```

**Hashmap interface** — every collision strategy exposes the same shape
(`create`, `insert`, `get`, `delete`, `destroy`), and takes a hash function
pointer as a dependency at `create()` time rather than hardcoding one. This is
what makes "every hash function × every hashmap" possible without special-casing.

**Directory layout:**

```
src/
  guest.h                    Guest struct, shared record type
  direct_addressing.{c,h}    Phase 1 (done)
  naive_hash.{c,h}           the broken modulo baseline (Phase 1→2 bridge)
  <chaining, probing, ...>   one file per collision strategy
hashfuncs/
  number_hashfuncs/          one file per numeric hash function
  string_hashfuncs/          one file per string hash function
tests/                       one test file per technique (TDD)
data/
  hotel_california.psv
docs/
  PLAN.md                    this file
main.c                       loads dataset, runs the matrix, prints spec sheet
README.md                    the educational document (vocabulary, diagrams, real results)
```

**Doc/code split convention:** conceptual explanations (why a technique works,
its constraints, tradeoffs) live in the README, one section per technique,
named to match the corresponding file/function. Code stays lean — clear
naming plus at most a one-line comment pointing back to the README section.
No duplicated explanations between the two.

## 5. Hash Function Catalog

### Numeric keys

| # | Name | Idea | Phase |
|---|---|---|---|
| 1 | Direct addressing | `guest_number - 1` — not really a hash; requires capacity == key range | 1 (done) |
| 2 | Modulo hashing | `guest_number % capacity` — simplest true hash; naive version overwrites on collision (the "broken baseline") | 1→2 |
| 3 | Multiplicative (Fibonacci) hashing | `(key * A) >> shift`, A derived from the golden ratio — better distribution than modulo when capacity isn't prime | 3 |
| 4 | Integer bit-mixing | splitmix/Murmur-finalizer-style multiply-xor-shift chain — for keys that aren't naturally well distributed (e.g. sparse `phone`) | 3 (stretch) |

### String keys

| # | Name | Idea | Phase |
|---|---|---|---|
| 1 | Sum of characters | naive baseline — deliberately bad, collides on anagrams | 2 |
| 2 | djb2 | `hash = hash*33 + c`, polynomial rolling hash (Horner's method) | 2 |
| 3 | FNV-1a | `hash = (hash XOR c) * prime` | 2 |
| 4 | Murmur3 (simplified) | multiply/rotate/mix, higher-quality modern non-cryptographic hash | 4 (stretch) |
| 5 | SipHash | DoS-resistant, used by Python/Rust dict implementations against adversarial collision attacks — mention in README as real-world context, implement only if time allows | stretch, optional |

**Design note (from the string-hashing discussion):** quality comes from
*position-sensitive* combination — folding each character in via multiply
(or multiply+XOR) as you go, in one pass — not from summing characters first
and hashing the sum afterward. A second hashing pass cannot recover positional
information a naive first pass already threw away. Use `unsigned` accumulators
so overflow wraparound is well-defined behavior, not UB.

## 6. Hashmap Catalog (collision resolution strategies)

| # | Name | Collision handling | Phase |
|---|---|---|---|
| 1 | Naive direct-mapped array | none — second insert silently overwrites first | 1 |
| 2 | Separate chaining | linked list per bucket | 2 |
| 3 | Linear probing | walk forward `+1, +2, ...` | 3 |
| 4 | Double hashing | step size from a second hash function | 3 |
| 5 | Dynamic resizing | layered on top of (2)–(4): rehash when load factor crosses ~0.7 | 4 |
| 6 | Robin Hood hashing | open addressing variant, evicts the entry closer to its ideal slot | 5 (stretch) |
| 7 | Cuckoo hashing | two tables/hashes, evicting relocation, worst-case O(1) lookup | 5 (stretch) |
| 8 | Perfect hashing | precomputed collision-free hash for the static 1,000-row dataset | 5 (stretch) |

## 7. Keying Strategies (map vs. multimap)

| Key | Type | Uniqueness | Structure |
|---|---|---|---|
| `guest_number` | numeric | unique, dense | map |
| `name` | string | near-unique (30 deliberate duplicate entries) | map |
| `phone` | numeric | unique but sparse | map |
| surname (parsed from `name`) | string | shared (common surnames repeat across guests) | multimap |

Build the map exercises first (phases 1–4), add the multimap exercise in
phase 5 by reusing chaining's bucket-list structure — a multimap is nearly
free once chaining exists. Surname (parsed out of `name`) is the natural
multimap key: many guests share a surname, so one key → a list of guests.

## 8. CLI Design

**`main`'s only job: print a spec sheet.** Run every (hashmap × hash-function)
combination against the full dataset and print one plain table — one row per
combination, columns being the benchmark metrics from §9. That's it. No lesson
flow, no animation, no color, no query/walkthrough modes. Example shape:

```
hashmap        hash        load   collisions  avg_probe  worst_probe  insert_ms  lookup_ms  bytes
chaining       djb2        4.00   612         2.3        14           0.9        0.8        ...
chaining       sum_chars   4.00   1840        5.1        41           1.2        1.4        ...
linear_probe   djb2        0.70   —           3.8        22           1.1        0.9        ...
double_hash    djb2        0.70   —           1.9        6            1.0        0.7        ...
```

**Optional flag-driven single run** (`--map chaining --hash fnv1a --key name`)
— runs one combination instead of the whole matrix. Nice-to-have for
debugging while building, and real CLI/argument-parsing practice, but not
required for v1. If it adds friction, skip it and just run the full matrix.

**Explicitly dropped from v1:** live bucket-fill bars, ANSI color/collision
flashes, scripted narrated walkthrough, and the interactive probe-sequence
query mode. These were the "engaging visual CLI" idea, now deprioritized —
the README carries all teaching; the CLI only emits numbers.

## 9. Benchmark Metrics

Track these across every hashmap × hash-function run, logged to CSV for the
README's results section:

- Collision rate / average & worst-case probe length
- Load factor vs. lookup performance curve
- Memory overhead (pointers per entry for chaining vs. empty slots for open addressing)
- Deletion cost (trivial in chaining, tombstones required in open addressing)
- Wall-clock time, not just comparison counts (cache locality differs between chaining and open addressing even at equal big-O)
- Hash function quality isolated from collision strategy (same hashmap, swap only the hash function)
- Resize cost spike, amortized over subsequent inserts

## 10. Phased Build Checklist

- [x] **Phase 0** — Dataset built (`data/hotel_california.psv`, 1,000 rows)
- [x] **Phase 1** — Direct addressing: `guest_number` key, capacity 1000,
      zero collisions. Loads full dataset from file, insert/lookup tested (TDD).
- [ ] **Phase 1.5** — Naive modulo hashmap (`guest_number % capacity`, one
      guest/slot, overwrites on collision). The "broken baseline": load guests,
      check in guest 1001 → collides with guest 1 → data loss shown. Motivates
      chaining. *(current step)*
- [ ] **Phase 2** — Separate chaining (linked list per bucket, `malloc`/`free`)
      + first string hash functions (sum-of-chars, djb2, FNV-1a), keyed by `name`
- [ ] **Phase 3** — Open addressing (linear probing, double hashing) +
      multiplicative/Fibonacci numeric hashing
- [ ] **Phase 4** — Dynamic resizing layered onto chaining and/or double hashing
- [ ] **Phase 5** — Multimap keyed by surname (parsed from `name`), reusing
      chaining's bucket-list structure
- [ ] **Phase 6** — CLI wiring: run the full (hashmap × hash-function) matrix
      and print a plain spec-sheet table (§8). No visuals/lesson flow.
- [ ] **Phase 7** — README write-up: vocabulary, diagrams, and real benchmark
      numbers pulled from an actual run (not invented)
- [ ] **Phase 8 (optional stretch)** — Robin Hood/Cuckoo hashing, Murmur3/
      SipHash, static (no-backend) results webpage

## 11. Explicitly Deferred / Out of Scope

Recorded so future-us remembers *why*, not just *that*:

- **Titanic dataset (earlier direction, abandoned)** — was real, verified, and
  emotionally compelling, but insert-only: nobody ever "leaves" the Titanic, so
  it can't motivate deletion, tombstones, or grow/shrink resizing. Pivoted to the
  synthetic Hotel California so check-in/check-out gives those operations a
  natural home.
- **Credit-card numbers as a column (rejected)** — would have given the sparse
  large-key-space we wanted, but a `name|card_number` file in a public repo
  looks like a breach dump (bad for a portfolio repo, could trip secret
  scanning). Used `phone` instead — same sparse-key property, no baggage.
- **JSON data format** — would require a real parser (or a dependency) before
  any hashmap code exists; deferred in favor of pipe-delimited text.
- **Full interactive web sandbox** (drag-and-drop hash function / dataset /
  map in-browser) — genuinely a second project (JS/WASM), competes directly
  with C-learning time. Revisit only after phases 1–7 are done and there's
  still appetite for it.
