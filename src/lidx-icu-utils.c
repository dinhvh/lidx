#include "lidx-icu-utils.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "lidx-utils.h"

// Transliteration helpers.

typedef struct XReplaceable {
  UChar* text;    /* MUST BE null-terminated */
} XReplaceable;

static void InitXReplaceable(XReplaceable* rep, const UChar* str, int length)
{
  if (length == 0) {
    length = u_strlen(str);
  }
  rep->text = (UChar*) malloc(sizeof(* rep->text) * (length + 1));
  rep->text[length] = 0;
  u_strncpy(rep->text, str, length);
}

static void FreeXReplaceable(XReplaceable* rep)
{
  if (rep->text != NULL) {
    free(rep->text);
    rep->text = NULL;
  }
}

/* UReplaceableCallbacks callback */
static int32_t Xlength(const UReplaceable* rep)
{
  const XReplaceable* x = (const XReplaceable*)rep;
  return u_strlen(x->text);
}

/* UReplaceableCallbacks callback */
static UChar XcharAt(const UReplaceable* rep, int32_t offset)
{
  const XReplaceable* x = (const XReplaceable*)rep;
  return x->text[offset];
}

/* UReplaceableCallbacks callback */
static UChar32 Xchar32At(const UReplaceable* rep, int32_t offset)
{
  const XReplaceable* x = (const XReplaceable*)rep;
  return x->text[offset];
}

/* UReplaceableCallbacks callback */
static void Xreplace(UReplaceable* rep, int32_t start, int32_t limit,
    const UChar* text, int32_t textLength)
{
  XReplaceable* x = (XReplaceable*)rep;
  int32_t newLen = Xlength(rep) + limit - start + textLength;
  UChar* newText = (UChar*) malloc(sizeof(UChar) * (newLen+1));
  u_strncpy(newText, x->text, start);
  u_strncpy(newText + start, text, textLength);
  u_strcpy(newText + start + textLength, x->text + limit);
  free(x->text);
  x->text = newText;
}

/* UReplaceableCallbacks callback */
static void Xcopy(UReplaceable* rep, int32_t start, int32_t limit, int32_t dest)
{
  XReplaceable* x = (XReplaceable*)rep;
  int32_t newLen = Xlength(rep) + limit - start;
  UChar* newText = (UChar*) malloc(sizeof(UChar) * (newLen+1));
  u_strncpy(newText, x->text, dest);
  u_strncpy(newText + dest, x->text + start, limit - start);
  u_strcpy(newText + dest + limit - start, x->text + dest);
  free(x->text);
  x->text = newText;
}

/* UReplaceableCallbacks callback */
static void Xextract(UReplaceable* rep, int32_t start, int32_t limit, UChar* dst)
{
  XReplaceable* x = (XReplaceable*)rep;
  int32_t len = limit - start;
  u_strncpy(dst, x->text, len);
}

static void InitXReplaceableCallbacks(UReplaceableCallbacks* callbacks)
{
  callbacks->length = Xlength;
  callbacks->charAt = XcharAt;
  callbacks->char32At = Xchar32At;
  callbacks->replace = Xreplace;
  callbacks->extract = Xextract;
  callbacks->copy = Xcopy;
}


// init and deinit.

static UReplaceableCallbacks s_xrepVtable;
static UTransliterator * s_trans = NULL;
static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static int s_initialized = 0;

void lidx_init_icu_utils(void)
{
  pthread_mutex_lock(&s_lock);
  if (!s_initialized) {
    UChar urules[1024];
    UErrorCode status = U_ZERO_ERROR;
    u_strFromUTF8(urules, sizeof(urules), NULL, "Any-Latin; NFD; Lower; [:nonspacing mark:] remove; nfc", -1, &status);
    LIDX_ASSERT(status == U_ZERO_ERROR);
  
    UParseError parseError;
    s_trans = utrans_openU(urules, -1, UTRANS_FORWARD,
    NULL, -1, &parseError, &status);
    LIDX_ASSERT(status == U_ZERO_ERROR);
  
    InitXReplaceableCallbacks(&s_xrepVtable);
    s_initialized = 1;
  }
  pthread_mutex_unlock(&s_lock);
}

void lidx_deinit_icu_utils(void)
{
  utrans_close(s_trans);
}


// UTF <-> UTF16

UChar * lidx_from_utf8(const char * word)
{
  int len = strlen(word);
  UChar * uword = (UChar *) malloc(sizeof(* uword) * (len + 1));
  uword[len] = 0;
  UErrorCode status = U_ZERO_ERROR;
  u_strFromUTF8(uword, len + 1, NULL, word, len, &status);
  return uword;
}

char * lidx_to_utf8(const UChar * word)
{
  int len = u_strlen(word);
  char * utf8word = (char *) malloc(len * 6 + 1);
  utf8word[len] = 0;
  UErrorCode status = U_ZERO_ERROR;
  u_strToUTF8(utf8word, len * 6 + 1, NULL, word, len, &status);
  return utf8word;
}

// transliterate to ASCII

char * lidx_transliterate(const UChar * text, int length)
{
  if (length == -1) {
    length = u_strlen(text);
  }
  
  XReplaceable xrep;
  InitXReplaceable(&xrep, text, length);
  UErrorCode status = U_ZERO_ERROR;
  
  int32_t limit = length;
  utrans_trans(s_trans, (UReplaceable *) &xrep, &s_xrepVtable, 0, &limit, &status);
  if (status != U_ZERO_ERROR) {
    goto free_xrep;
  }
  
  char * result = lidx_to_utf8(xrep.text);
  FreeXReplaceable(&xrep);

  return result;

  free_xrep:
  FreeXReplaceable(&xrep);
  err:
  return NULL;
}
