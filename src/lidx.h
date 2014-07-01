#ifndef LIDX_H

#define LIDX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdlib.h>

#if defined(__CHAR16_TYPE__)
typedef __CHAR16_TYPE__ UChar;
#else
typedef uint16_t UChar;
#endif

typedef struct lidx lidx;

// prefix provides the best performance, two other options
// have poor performance.
typedef enum lidx_search_kind {
  lidx_search_kind_prefix,
  lidx_search_kind_substr,
  lidx_search_kind_suffix,
} lidx_search_kind;

// Create a new indexer.
lidx * lidx_new(void);

// Release resource of the new indexer.
void lidx_free(lidx * index);

// Open the indexer.
int lidx_open(lidx * index, const char * filename);

// Close the indexer.
void lidx_close(lidx * index);

// Adds a UTF-8 document to the indexer.
int lidx_set(lidx * index, uint64_t doc, const char * text);

// Adds an unicode document to the indexer.
int lidx_u_set(lidx * index, uint64_t doc, const UChar * utext);

// Remove a document from the indexer.
int lidx_remove(lidx * index, uint64_t doc);

// Search a UTF-8 token in the indexer.
int lidx_search(lidx * index, const char * token, lidx_search_kind kind,
    uint64_t ** p_docsids, size_t * p_count);

// Search a unicode token in the indexer.
int lidx_u_search(lidx * index, const UChar * utoken, lidx_search_kind kind,
    uint64_t ** p_docsids, size_t * p_count);

#ifdef __cplusplus
}
#endif

#endif
