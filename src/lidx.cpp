#include "lidx.h"

#include <stdlib.h>

#include <leveldb/db.h>
#include <leveldb/status.h>
#include <leveldb/write_batch.h>

#include "lidx-utils.h"
#include "lidx-icu-utils.h"
#include "lidx-encode.h"

#include <set>
#include <map>

static int db_put(lidx * index, std::string & key, std::string & value);
static int db_get(lidx * index, std::string & key, std::string * p_value);
static int db_delete(lidx * index, std::string & key);
static int db_flush(lidx * index);

// . -> next word id
// ,[docid] -> [words ids]
// /[word id] -> word
// word -> [word id], [docs ids]

struct lidx {
  leveldb::DB * lidx_db;
  std::map<std::string, std::string> * lidx_buffer;
  std::set<std::string> * lidx_buffer_dirty;
  std::set<std::string> * lidx_deleted;
};

lidx * lidx_new(void)
{
  lidx_init_icu_utils();
  lidx * result = (lidx *) calloc(1, sizeof(* result));
  result->lidx_buffer = new std::map<std::string, std::string>();
  result->lidx_buffer_dirty = new std::set<std::string>();
  result->lidx_deleted = new std::set<std::string>();
  return result;
}

void lidx_free(lidx * index)
{
  delete index->lidx_buffer;
  delete index->lidx_buffer_dirty;
  delete index->lidx_deleted;
  free(index);
}

int lidx_open(lidx * index, const char * filename)
{
  leveldb::Options options;
  leveldb::Status status;

  options.create_if_missing = true;
  status = leveldb::DB::Open(options, filename, &index->lidx_db);
  if (!status.ok()) {
    return -1;
  }
  
  return 0;
}

void lidx_close(lidx * index)
{
  if (index->lidx_db == NULL) {
    return;
  }
  db_flush(index);
  delete index->lidx_db;
  index->lidx_db = NULL;
}

int lidx_flush(lidx * index)
{
  return db_flush(index);
}

//int lidx_set(lidx * index, uint64_t doc, const char * text);
// text -> wordboundaries -> transliterated word -> store word with new word id
// word -> append doc id to docs ids
// store doc id -> words ids

static int tokenize(lidx * index, uint64_t doc, const UChar * text);
static int add_to_indexer(lidx * index, uint64_t doc, const char * word,
    std::set<uint64_t> & wordsids_set);

int lidx_set(lidx * index, uint64_t doc, const char * text)
{
  UChar * utext = lidx_from_utf8(text);
  int result = lidx_u_set(index, doc, utext);
  free((void *) utext);
  return result;
}

int lidx_u_set(lidx * index, uint64_t doc, const UChar * utext)
{
  int r = lidx_remove(index, doc);
  if (r < 0) {
    return r;
  }
  r = tokenize(index, doc, utext);
  if (r < 0) {
    return r;
  }
  return 0;
}

static int tokenize(lidx * index, uint64_t doc, const UChar * text)
{
  int result = 0;
  UErrorCode status;
  status = U_ZERO_ERROR;
  UBreakIterator * iterator = ubrk_open(UBRK_WORD, NULL, text, u_strlen(text), &status);
  LIDX_ASSERT(status <= U_ZERO_ERROR);
  
  int32_t left = 0;
  int32_t right = 0;
  int word_kind = 0;
  ubrk_first(iterator);
  std::set<uint64_t> wordsids_set;
  
  while (1) {
    left = right;
    right = ubrk_next(iterator);
    if (right == UBRK_DONE) {
      break;
    }

    word_kind = ubrk_getRuleStatus(iterator);
    if (word_kind == 0) {
      // skip punctuation and space.
      continue;
    }

    char * transliterated = lidx_transliterate(&text[left], right - left);
    if (transliterated == NULL) {
      continue;
    }
    int r = add_to_indexer(index, doc, transliterated, wordsids_set);
    if (r < 0) {
      result = r;
      break;
    }

    free(transliterated);
  }
  ubrk_close(iterator);
  
  std::string key(",");
  lidx_encode_uint64(key, doc);
  
  std::string value_str;
  for(std::set<uint64_t>::iterator wordsids_set_iterator = wordsids_set.begin() ; wordsids_set_iterator != wordsids_set.end() ; ++ wordsids_set_iterator) {
    lidx_encode_uint64(value_str, * wordsids_set_iterator);
  }
  int r = db_put(index, key, value_str);
  if (r < 0) {
    return r;
  }
  
  return result;
}

static int add_to_indexer(lidx * index, uint64_t doc, const char * word,
    std::set<uint64_t> & wordsids_set)
{
  std::string word_str(word);
  std::string value;
  uint64_t wordid;
  
  int r = db_get(index, word_str, &value);
  if (r < -1) {
    return -1;
  }
  if (r == 0) {
    // Adding doc id to existing entry.
    lidx_decode_uint64(value, 0, &wordid);
    lidx_encode_uint64(value, doc);
    int r = db_put(index, word_str, value);
    if (r < 0) {
      return r;
    }
  }
  else /* r == -1 */ {
    // Not found.
    
    // Creating an entry.
    // store word with new id
    
    // read next word it
    std::string str;
    std::string nextwordidkey(".");
    int r = db_get(index, nextwordidkey, &str);
    if (r == -1) {
      wordid = 0;
    }
    else if (r < 0) {
      return -1;
    }
    else {
      lidx_decode_uint64(str, 0, &wordid);
    }
    
    // write next word id
    std::string value;
    uint64_t next_wordid = wordid;
    next_wordid ++;
    lidx_encode_uint64(value, next_wordid);
    r = db_put(index, nextwordidkey, value);
    if (r < 0) {
      return r;
    }
    
    std::string value_str;
    lidx_encode_uint64(value_str, wordid);
    lidx_encode_uint64(value_str, doc);
    r = db_put(index, word_str, value_str);
    if (r < 0) {
      return r;
    }
    
    std::string key("/");
    lidx_encode_uint64(key, wordid);
    r = db_put(index, key, word_str);
    if (r < 0) {
      return r;
    }
  }
  
  wordsids_set.insert(wordid);

  return 0;
}

//int lidx_remove(lidx * index, uint64_t doc);
// docid -> words ids -> remove docid from word
// if docs ids for word is empty, we remove the word id

static std::string get_word_for_wordid(lidx * index, uint64_t wordid);
static int remove_docid_in_word(lidx * index, std::string word, uint64_t doc);
static int remove_word(lidx * index, std::string word, uint64_t wordid);

int lidx_remove(lidx * index, uint64_t doc)
{
  std::string key(",");
  lidx_encode_uint64(key, doc);
  std::string str;
  int r = db_get(index, key, &str);
  if (r == -1) {
    // do nothing
  }
  else if (r < 0) {
    return -1;
  }
  
  size_t position = 0;
  while (position < str.size()) {
    uint64_t wordid;
    position = lidx_decode_uint64(str, position, &wordid);
    std::string word = get_word_for_wordid(index, wordid);
    if (word.size() == 0) {
      continue;
    }
    int r = remove_docid_in_word(index, word, doc);
    if (r < 0) {
      return -1;
    }
  }
  
  return 0;
}

static std::string get_word_for_wordid(lidx * index, uint64_t wordid)
{
  std::string wordidkey("/");
  lidx_encode_uint64(wordidkey, wordid);
  std::string str;
  int r = db_get(index, wordidkey, &str);
  if (r < 0) {
    return std::string();
  }
  return str;
}

static int remove_docid_in_word(lidx * index, std::string word, uint64_t doc)
{
  std::string str;
  int r = db_get(index, word, &str);
  if (r == -1) {
    return 0;
  }
  else if (r < 0) {
    return -1;
  }
  
  uint64_t wordid;
  std::string buffer;
  size_t position = 0;
  position = lidx_decode_uint64(str, position, &wordid);
  while (position < buffer.size()) {
    uint64_t current_docid;
    position = lidx_decode_uint64(str, position, &current_docid);
    if (current_docid != doc) {
      lidx_encode_uint64(buffer, current_docid);
    }
  }
  if (buffer.size() == 0) {
    // remove word entry
    int r = remove_word(index, word, wordid);
    if (r < 0) {
      return -1;
    }
  }
  else {
    // update word entry
    int r = db_put(index, word, buffer);
    if (r < 0) {
      return r;
    }
  }
  
  return 0;
}

static int remove_word(lidx * index, std::string word, uint64_t wordid)
{
  std::string wordidkey("/");
  lidx_encode_uint64(wordidkey, wordid);
  int r;
  r = db_delete(index, wordidkey);
  if (r < 0) {
    return -1;
  }
  r = db_delete(index, word);
  if (r < 0) {
    return -1;
  }
  
  return 0;
}

//int lidx_search(lidx * index, const char * token);
// token -> transliterated token -> docs ids

int lidx_search(lidx * index, const char * token, lidx_search_kind kind, uint64_t ** p_docsids, size_t * p_count)
{
  int result;
  UChar * utoken = lidx_from_utf8(token);
  result = lidx_u_search(index, utoken, kind, p_docsids, p_count);
  free((void *) utoken);
  return result;
}

int lidx_u_search(lidx * index, const UChar * utoken, lidx_search_kind kind,
    uint64_t ** p_docsids, size_t * p_count)
{
  db_flush(index);
  
  char * transliterated = lidx_transliterate(utoken, -1);
  unsigned int transliterated_length = strlen(transliterated);
  std::set<uint64_t> result_set;
  
  leveldb::ReadOptions options;
  leveldb::Iterator * iterator = index->lidx_db->NewIterator(options);
  if (kind == lidx_search_kind_prefix) {
    iterator->Seek(transliterated);
  }
  else {
    iterator->SeekToFirst();
  }
  while (iterator->Valid()) {
    int add_to_result = 0;
    
    if (iterator->key().starts_with(".") || iterator->key().starts_with(",") || iterator->key().starts_with("/")) {
      std::string key = iterator->key().ToString();
      iterator->Next();
      continue;
    }
    if (kind == lidx_search_kind_prefix) {
      if (!iterator->key().starts_with(transliterated)) {
        break;
      }
      add_to_result = 1;
    }
    else if (kind == lidx_search_kind_substr) {
      const char * data = iterator->key().data();
      size_t size = iterator->key().size();
      char * str = (char *) malloc(size + 1);
      memcpy(str, data, size);
      str[size] = 0;
      if (iterator->key().ToString().find(transliterated) != -1) {
        add_to_result = 1;
      }
    }
    else if (kind == lidx_search_kind_suffix) {
      std::string key = iterator->key().ToString();
      if ((key.length() >= transliterated_length) &&
        (key.compare(key.length() - transliterated_length, transliterated_length, transliterated) == 0)) {
        add_to_result = 1;
      }
    }
    if (add_to_result) {
      size_t position = 0;
      uint64_t wordid;
      std::string value_str = iterator->value().ToString();
      position = lidx_decode_uint64(value_str, position, &wordid);
      while (position < value_str.size()) {
        uint64_t docid;
        position = lidx_decode_uint64(value_str, position, &docid);
        result_set.insert(docid);
      }
    }
    
    iterator->Next();
  }
  free(transliterated);
  
  uint64_t * result = (uint64_t *) calloc(result_set.size(), sizeof(* result));
  unsigned int count = 0;
  for(std::set<uint64_t>::iterator set_iterator = result_set.begin() ; set_iterator != result_set.end() ; ++ set_iterator) {
    result[count] = * set_iterator;
    count ++;
  }
  
  * p_docsids = result;
  * p_count = count;
  
  return 0;
}

static int db_put(lidx * index, std::string & key, std::string & value)
{
  index->lidx_deleted->erase(key);
  (* index->lidx_buffer)[key] = value;
  index->lidx_buffer_dirty->insert(key);

  return 0;
}

static int db_get(lidx * index, std::string & key, std::string * p_value)
{
  if (index->lidx_deleted->find(key) != index->lidx_deleted->end()) {
    return -1;
  }

  if (index->lidx_buffer->find(key) != index->lidx_buffer->end()) {
    * p_value = (* index->lidx_buffer)[key];
    return 0;
  }
  
  leveldb::ReadOptions read_options;
  leveldb::Status status = index->lidx_db->Get(read_options, key, p_value);
  if (status.IsNotFound()) {
    return -1;
  }
  if (!status.ok()) {
    return -2;
  }
  (* index->lidx_buffer)[key] = * p_value;
  return 0;
}

static int db_delete(lidx * index, std::string & key)
{
  index->lidx_deleted->insert(key);
  index->lidx_buffer_dirty->erase(key);
  index->lidx_buffer->erase(key);
  return 0;
}

static int db_flush(lidx * index)
{
  if ((index->lidx_buffer_dirty->size() == 0) && (index->lidx_deleted->size() == 0)) {
    return 0;
  }
  leveldb::WriteBatch batch;
  for(std::set<std::string>::iterator set_iterator = index->lidx_buffer_dirty->begin() ; set_iterator != index->lidx_buffer_dirty->end() ; ++ set_iterator) {
    std::string key = * set_iterator;
    std::string value = (* index->lidx_buffer)[key];
    batch.Put(key, value);
  }
  for(std::set<std::string>::iterator set_iterator = index->lidx_deleted->begin() ; set_iterator != index->lidx_deleted->end() ; ++ set_iterator) {
    std::string key = * set_iterator;
    batch.Delete(key);
  }
  leveldb::WriteOptions write_options;
  leveldb::Status status = index->lidx_db->Write(write_options, &batch);
  if (!status.ok()) {
    return -1;
  }
  return 0;
}