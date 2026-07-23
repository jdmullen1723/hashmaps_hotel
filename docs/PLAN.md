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

**Decision: CLI-first, README-as-the-lesson.** All effort goes into the C core
and a genuinely engaging terminal experience. A web layer is explicitly
out of scope for v1 (see §11). The reasoning: a polished CLI + a README with
real benchmark data is a strong, differentiated portfolio piece on its own,
and every hour spent on a web frontend is an hour not spent on the C/hashmap
fundamentals that are the actual point.

## 2. Dataset

**File:** `data/titanic.psv` — pipe-delimited, 1,309 passenger rows + 1 header row.

**Source:** the full `titanic3` dataset (Vanderbilt Biostatistics — Frank Harrell
Jr. & Thomas Cason, sourced from Encyclopedia Titanica research), not the
smaller 891-row Kaggle ML-competition split. Pulled from
`https://hbiostat.org/data/repo/titanic3.csv`, cross-referenced against
`https://www.openml.org/d/40945`. `survived` is complete for all 1,309 rows —
verified, zero missing.

**Schema:**

```
id|pclass|survived|name|sex|age|sibsp|parch|ticket|fare|cabin|embarked|boat|body|home_dest
```

| Column | Type | Missing | Notes |
|---|---|---|---|
| `id` | int | 0 | **synthesized by us** — row number 1–1309, not a real historical identifier |
| `pclass` | int | 0 | 1st/2nd/3rd class |
| `survived` | bool (0/1) | 0 | 1 = survived, 0 = died — verified against real Allison-family records |
| `name` | string | 0 | left exactly as sourced — see naming note below |
| `sex` | string | 0 | `male`/`female` |
| `age` | float | 263 | ~20% missing, needs a null convention |
| `sibsp` | int | 0 | siblings/spouses aboard |
| `parch` | int | 0 | parents/children aboard |
| `ticket` | string | 0 | shared by families/groups — multimap key candidate |
| `fare` | float | 1 | |
| `cabin` | string | 1014 | mostly blank — real historical gap, not a data error |
| `embarked` | string | 2 | `S`/`C`/`Q` (Southampton/Cherbourg/Queenstown) + 2 blank |
| `boat` | string | 823 | lifeboat number, filled only for survivors |
| `body` | string | 1188 | recovery body number, filled only for identified recovered bodies |
| `home_dest` | string | 564 | hometown/destination |

**Key decisions made:**

- **`id` is synthetic**, not scraped — the source has no numeric identifier at
  all. It exists purely so we have a clean numeric key to hash against.
- **`name` is untouched**, including married women recorded under their
  husband's formal name (e.g. `Andersson, Mrs. Anders Johan (Alfrida
  Konstantia Brogren)`). Kept deliberately for the collision-testing value:
  spouses produce near-identical key strings differing by only a couple of
  characters, which is a good stress test for hash avalanche behavior.
- **Pipe-delimited, not JSON** — avoids needing a real CSV/JSON parser in C
  before any hashmap code exists. Verified no field contains a literal `|`.
- **Crew and non-boarding "phantom" passengers were investigated and
  deliberately excluded** — see §11 for why.

**Real collision material already in the data** (not contrived):

- Two genuine duplicate names: `Connolly, Miss. Kate` (two different women,
  ages 22 and 30) and `Kelly, Mr. James` (two different men, ages 34.5 and 44).
- 134 distinct ticket numbers shared by 2–7 passengers (e.g. the Andersson
  family, 7 people on ticket `347082`).
- Surnames shared well beyond marriage: Andersson (9), Sage (7), Johnson (6),
  Panula (6), Goodwin (6).

## 3. Vocabulary

Canonical definitions — use these consistently in code comments, README, and
the CLI's own output text.

- **Key** — the lookup input (e.g. `id`, `name`, `ticket`).
- **Value** — the data associated with the key (the rest of the passenger record).
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

**Entry struct** — one shared passenger record type in `src/common/`, used by
every hashmap implementation regardless of collision strategy.

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
  common/       Entry struct, shared types
  hashfuncs/    one file per hash function (numeric/ and string/ subfolders)
  hashmaps/     one file per collision strategy
  cli/          argument parsing, lesson-mode script, visual rendering
data/
  titanic.psv
docs/
  PLAN.md       this file
README.md       the educational document (vocabulary, diagrams, real results)
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
| 1 | Direct addressing | `id - 1` — not really a hash; requires capacity == key range | 1 |
| 2 | Modulo hashing | `id % capacity` — simplest true hash | 1 |
| 3 | Multiplicative (Fibonacci) hashing | `(id * A) >> shift`, A derived from the golden ratio — better distribution than modulo when capacity isn't prime | 3 |
| 4 | Integer bit-mixing | splitmix/Murmur-finalizer-style multiply-xor-shift chain — for keys that aren't naturally well distributed | 3 (stretch) |

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
| 8 | Perfect hashing | precomputed collision-free hash for the static 1,309-row dataset | 5 (stretch) |

## 7. Keying Strategies (map vs. multimap)

| Key | Type | Uniqueness | Structure |
|---|---|---|---|
| `id` | numeric | unique | map |
| `name` | string | near-unique (2 real duplicate pairs) | map |
| `ticket` | string | shared (134 tickets, up to 7 people) | multimap |
| surname (parsed from `name`) | string | shared (up to 9 people) | multimap |

Build the map exercises first (phases 1–4), add the multimap exercises in
phase 5 by reusing chaining's bucket-list structure — a multimap is nearly
free once chaining exists.

## 8. CLI Design

Two entry points into the same underlying hashmap × hash-function matrix:

**Default (`./main`, no args)** — a scripted lesson-flow walkthrough: runs
each technique in build order against the full dataset, printing a short
explanation and a live visual per stage, ending in a leaderboard comparing
every combination run. Zero-knowledge-required — this is the "just run it"
demo experience.

**Flag-driven (`./main --map chaining --hash fnv1a --key ticket --multimap`)**
— runs exactly one specified combination. This is the actual
exploration/debugging tool used while building, and it's real CLI/argument-
parsing practice in C.

**Visual elements** (terminal-native, ASCII/ANSI — not image graphs):

- Live bucket-fill bars (`███░░░`) updating as inserts happen.
- ANSI color flash on collision events.
- Final leaderboard table: strategy, hash fn, avg probe, worst probe, time.
- Query mode: look up one real passenger (e.g. Kate Connolly — a genuine
  collision case) and print the actual probe sequence taken to find them.

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

- [x] **Phase 0** — Dataset built (`data/titanic.psv`, 1,309 rows, verified)
- [ ] **Phase 1** — Direct addressing baseline: `id` key, capacity fixed at
      1309, confirm zero collisions. *(current step — in progress)*
- [ ] **Phase 2** — Separate chaining + first string hash functions
      (sum-of-chars, djb2, FNV-1a), keyed by `name`
- [ ] **Phase 3** — Open addressing (linear probing, double hashing) +
      multiplicative/Fibonacci numeric hashing
- [ ] **Phase 4** — Dynamic resizing layered onto chaining and/or double hashing
- [ ] **Phase 5** — Multimap keyed by `ticket`/surname, reusing chaining's
      bucket-list structure
- [ ] **Phase 6** — CLI wiring: two entry-point modes, visual rendering,
      leaderboard, single-passenger query mode
- [ ] **Phase 7** — README write-up: vocabulary, diagrams, and real benchmark
      numbers pulled from an actual run (not invented)
- [ ] **Phase 8 (optional stretch)** — Robin Hood/Cuckoo hashing, Murmur3/
      SipHash, static (no-backend) results webpage

## 11. Explicitly Deferred / Out of Scope

Recorded so future-us remembers *why*, not just *that*:

- **Crew records** — every legitimate structured dataset (OpenML, Vanderbilt)
  explicitly excludes crew. Encyclopedia Titanica has crew bios but only as
  ~890 individual pages behind Cloudflare bot-protection — a separate
  scraping project, not a data-sourcing task.
- **Non-boarding "phantom" passengers** — real (50+ authenticated
  cancellations: J.P. Morgan, George Vanderbilt, Milton Hershey), but exists
  only as narrative prose on a bot-walled page, not structured data.
- **JSON data format** — would require a real parser (or a dependency) before
  any hashmap code exists; deferred in favor of pipe-delimited text.
- **Full interactive web sandbox** (drag-and-drop hash function / dataset /
  map in-browser) — genuinely a second project (JS/WASM), competes directly
  with C-learning time. Revisit only after phases 1–7 are done and there's
  still appetite for it.
