#ifndef LIDX_H

#define LIDX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>
#include <stdlib.h>

// We're using the same UChar as the ICU library.
#if defined(__CHAR16_TYPE__)
typedef __CHAR16_TYPE__ UChar;
#else
typedef uint16_t UChar;
#endif

typedef struct lidx lidx;

// prefix provides the best performance, two other options
// have poor performance.
typedef enum lidx_search_kind {
  lidx_search_kind_prefix, // Search documents that has strings that start with the given token.
  lidx_search_kind_substr, // Search documents that has strings that contain the given token.
  lidx_search_kind_suffix, // Search documents that has strings that end the given token.
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
// `doc`: document identifier (numerical identifier in a 64-bits range)
// `content`: content of the document in UTF-8 encoding.
int lidx_set(lidx * index, uint64_t doc, const char * text);

// Adds an unicode document to the indexer.
// `content`: content of the document in UTF-16 encoding.
int lidx_u_set(lidx * index, uint64_t doc, const UChar * utext);

// Removes a document from the indexer.
int lidx_remove(lidx * index, uint64_t doc);

// Searches a UTF-8 token in the indexer.
// `token`: string to search in UTF-8 encoding.
// `kind`: kind of matching to perform. See `lidx_search_kind`.
// The result is an array of documents IDs. The array is stored in `*p_docsids`.
// The number of items in the result array is stored in `*p_count`.
//
// The result array has to be freed using `free()`.
int lidx_search(lidx * index, const char * token, lidx_search_kind kind,
    uint64_t ** p_docsids, size_t * p_count);

// Searches a unicode token in the indexer.
// `token`: string to search in UTF-16 encoding.
int lidx_u_search(lidx * index, const UChar * utoken, lidx_search_kind kind,
    uint64_t ** p_docsids, size_t * p_count);

// Writes changes to disk if they are still pending in memory.
int lidx_flush(lidx * index);

#ifdef __cplusplus
}
#endif

#endif
