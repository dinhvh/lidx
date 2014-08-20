lidx
====

Unicode compatible full text indexer based on LevelDB.

Requirements
============
- LevelDB
- ICU library

How to use it
=============

```
lidx * indexer;
int r;
uint64_t * result;
size_t result_count;

indexer = lidx_new();
r = lidx_open(indexer, "index.lidx");
if (r < 0) {
  // error handling.
}

r = lidx_set(indexer, 0, "George Washington");
if (r < 0) {
  // error handling.
}

r = lidx_set(indexer, 1, "John Adams");
if (r < 0) {
  // error handling.
}

r = lidx_set(indexer, 2, "Thomas Jefferson");
if (r < 0) {
  // error handling.
}

r = lidx_set(indexer, 3, "George Michael");
if (r < 0) {
  // error handling.
}

r = lidx_set(indexer, 4, "George Méliès");
if (r < 0) {
  // error handling.
}

print("searching geor");
r = lidx_search(indexer, "geor", lidx_search_kind_suffix, &result, &result_count);
for(size_t i = 0 ; i < result_count ; i ++) {
  printf("found: %i\n", result[i]);
}
// returns 0, 3 and 4.
free(result);

print("searching mel");
r = lidx_search(indexer, "mel", lidx_search_kind_suffix, &result, &result_count);
for(size_t i = 0 ; i < result_count ; i ++) {
  printf("found: %i\n", result[i]);
}
// return 4
free(result);

lidx_close(indexer);
lidx_free(indexer);
```
