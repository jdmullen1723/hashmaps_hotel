This project is an exploration of Hashmaps and hashfunctions. 


## Why hashmaps?
There are a few ways we can store data values, and each way has different "look up" speeds; how fast it is to find a value within the data structure.

- Unsorted Array List
    -  O(n) lookup time
    - You must scan every entry to find the value you are looking for
- Sorted Array
    - O(log n) lookup time
    - Still just a contiguous array, but organized in specific fashion (ascending order for example). in the case of ascending order you can use binary search to find the values you want: drop in at index x, is the value I want bigger or smaller than x? Smaller? --> ok drop all values > x.
    - Inserts are O(n) because you must reorganize values to keep them sorted
- Hashmap
    - O(1) on average
    - Every value in your dataset is assigned a key. These are stored alongside each other in key-value pairs. 
    - A key is what you look up values by
    - A Hash Function maps a key to an integer. This integer can then be reduced to fit in the size of your array using the modulus operation (arrays index = hashed key integer % array size)
    - Sometimes multiple different keys can map to the same integer and hence the same array index. This is called a Collision, and it is one of the main problems that require solving when designing Hashmaps.

## Structure of this Repo:
In this project I will use a single dataset of key value pairs (perhaps more than pairs if I want to assign multiple values to a key later) and design different hashmaps to store these values. Then I will compare look up times, minimum storage capacity, collision rate, load factor, deletion cost, cache behavior and more metrics to compare the advantages and disadvantages of different hashmaps. 
I will use a single Hashfunction for all the different hashmap cases to simplify things. 

Notes: 
- Data uses passenger info from titanic as the situation. Passenger ID in dataset is created by us, not a real value from existing datset. It's just in order that the passengers are listed. The rest of the entries are scraped from the following datset: https://www.openml.org/d/40945
- I am learning C in this project, so some inline comments may be obvious to others or unnusual
- Guest name limited to 64 bytes --> so 63 usable characters and one terminator byte. It's one byte per letter. 
- There is a datafile with the whole guest list, and a single loader function which loads this file into an array str


## Direct Addressing
- Guests mapped by Guest Number, numbers 1 - 1000. Guest 1 goes to index 0, guest 2 to index 1 etc



## Building and Running

You'll need a C compiler — `gcc` or `clang` (on macOS these are the same tool, installed via the Xcode Command Line Tools: `xcode-select --install`).

From the project root, compile with:

    gcc -Wall -Wextra -Isrc -o main main.c src/hashmaps/*.c

Then run:

    ./main

**Notes:**
- `-Isrc` lets headers be included relative to the `src/` directory (e.g. `#include "hashmaps/direct_addressing/direct_addressing.h"`) from anywhere in the project.
- `-Wall -Wextra` turn on compiler warnings (recommended).
- Run `./main` **from the project root** — it reads `data/hotel_california.psv` via a relative path, so it must be launched from the directory that contains `data/`.
- As more source directories are added (e.g. `src/hashfuncs/`), include them in the compile command: `... src/hashmaps/*/*.c src/hashfuncs/*/*.c`
