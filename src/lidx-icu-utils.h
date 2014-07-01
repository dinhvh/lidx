#ifndef LIDX_ICU_UTILS_H

#define LIDX_ICU_UTILS_H

#include "unicode/utypes.h"
#include "unicode/uloc.h"
#include "unicode/utext.h"
#include "unicode/localpointer.h"
#include "unicode/parseerr.h"
#include "unicode/ubrk.h"
#include "unicode/urep.h"
#include "unicode/utrans.h"
#include "unicode/parseerr.h"
#include "unicode/uenum.h"
#include "unicode/uset.h"
#include "unicode/putil.h"
#include "unicode/uiter.h"
#include "unicode/ustring.h"

#ifdef __cplusplus
extern "C" {
#endif

void lidx_init_icu_utils(void);
void lidx_deinit_icu_utils(void);

UChar * lidx_from_utf8(const char * word);
char * lidx_to_utf8(const UChar * word);
char * lidx_transliterate(const UChar * text, int length);

#ifdef __cplusplus
}
#endif

#endif
