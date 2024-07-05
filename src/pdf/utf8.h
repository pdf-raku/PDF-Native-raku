#ifndef PDF_UTF8_H_
#define PDF_UTF8_H_

/*
  Adapted from: https://rosettacode.org/wiki/UTF-8_encode_and_decode#C
*/

#include "pdf.h"

/* All lengths are in bytes */
DLLEXPORT int utf8_code_len(const uint32_t cp); /* len of associated utf-8 char */
DLLEXPORT int utf8_char_len(const char ch);          /* len of utf-8 encoded char */

DLLEXPORT int utf8_from_code(uint32_t cp, uint8_t* buf);
DLLEXPORT uint32_t utf8_to_code(const uint8_t chr[4]);

#endif
