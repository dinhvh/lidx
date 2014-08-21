lidx
====

Unicode compatible full text indexer based on LevelDB.

Requirements
============
- LevelDB
- ICU library

How to build LevelDB for iOS
============================
```
$ cd leveldb
$ make PLATFORM=IOS
```

How to build ICU for iOS and Mac
================================
in `lidx/scripts`, you'll find two scripts `prepare-icu4c-ios.sh` and `prepare-icu4c-macos.sh`.
The results of the builds will be available in `lidx/scripts/builds/builds`.

How to use it
=============

```
lidx * indexer;
int r;
uint64_t * result;
size_t result_count;

// Opens the index.
indexer = lidx_new();
lidx_open(indexer, "index.lidx");

// Adds data to the index.
lidx_set(indexer, 0, "George Washington");
lidx_set(indexer, 1, "John Adams");
lidx_set(indexer, 2, "Thomas Jefferson");
lidx_set(indexer, 3, "George Michael");
lidx_set(indexer, 4, "George Méliès");

// Search "geor".
print("searching geor");
lidx_search(indexer, "geor", lidx_search_kind_suffix, &result, &result_count);
for(size_t i = 0 ; i < result_count ; i ++) {
  printf("found: %i\n", result[i]);
}
// returns 0, 3 and 4.
free(result);

// Search "mel".
print("searching mel");
lidx_search(indexer, "mel", lidx_search_kind_suffix, &result, &result_count);
for(size_t i = 0 ; i < result_count ; i ++) {
  printf("found: %i\n", result[i]);
}
// return 4
free(result);

lidx_close(indexer);
lidx_free(indexer);
```
