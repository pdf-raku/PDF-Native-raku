#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>
#include "pdf.h"
#include "pdf/types.h"
#include "pdf/write.h"
#include "pdf/utf8.h"
#include "pdf/_bufcat.h"

DLLEXPORT size_t pdf_write_bool(PDF_TYPE_BOOL val, char *out, size_t out_len) {
    if (out_len < (val ? 4 : 5)) return 0;
    strncpy(out, val ? "true" : "false", out_len);
    return strnlen(out, out_len);
}

DLLEXPORT size_t pdf_write_int(PDF_TYPE_INT64 val, char *out, size_t out_len) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%" PRId64, val);
  strncpy(out, buf, out_len);
  return strnlen(out, out_len);
}

DLLEXPORT size_t pdf_write_real(PDF_TYPE_REAL val, char *out, size_t out_len) {
  char   buf[32];
  char   *t;
  char   *dp;
  size_t n;

  const char* fmt = (val > 9999999 || -val > 9999999) ? "%.1f" : "%.5f";
  snprintf(buf, sizeof(buf), fmt, val);

  dp = strchr(buf, '.');
  if (dp) {
      /* omit trailing zeros and maybe '.' */
      for (t = dp + strlen(dp) - 1
               ; t >= dp && (*t == '0' || t == dp)
               ; t--);
      n = t - buf + 1;
  }
  else {
      n = strnlen(buf, sizeof(buf));
  }

  if (n > out_len) return 0;
  memcpy(out, buf, n);
  return n;
}

DLLEXPORT size_t pdf_write_literal(PDF_TYPE_STRING val, size_t in_len, char* out, size_t out_len) {

  PDF_TYPE_STRING in_p = val;
  PDF_TYPE_STRING in_end = val + in_len;
  size_t n = 0, m;

  if (out_len < 2) return 0;
  out[n++] = '(';

  while (in_p < in_end && n < out_len) {
    char c = *(in_p++);
    char esc[3] = {'\\', 0 , 0};
    const char* sym;

    if (c && (sym = strchr("\n\r\t\f\b" "\0" "nrtfb", c))) {
      // symbolic escape
      esc[1] = sym[6];
      n += (m = _bufcat(out+n, out_len-n, esc));
      if (m == 0) return 0;
    }
    else {
      if (c && strchr("\\()", c)) {
          // escape: '\\', '(', ')'
          if (n+1 >= out_len) return 0;
          out[n++] = '\\';
      }
      if (n >= out_len) return 0;
      out[n++] = c;
    }
  }

  if (n >= out_len) return 0;
  out[n++] = ')';
  return n;
}

static uint8_t hex_char(uint8_t c) {
  assert(c < 16);
  return ((c < 10)
          ? ('0' + c)
          : ('a' + (c - 10))
          );
}

DLLEXPORT size_t pdf_write_hex_string(PDF_TYPE_STRING val, size_t in_len, char* out, size_t out_len) {

  PDF_TYPE_STRING in_p = val;
  PDF_TYPE_STRING in_end = val + in_len;
  size_t n = 0;

  if (out_len < 2) return 0;
  out[n++] = '<';

  while (in_p < in_end) {
    uint8_t c = *(in_p++);
    if (n+1 >= out_len) return 0;
    out[n++] = hex_char(c / 16);
    out[n++] = hex_char(c % 16);
  }

  if (n >= out_len) return 0;
  out[n++] = '>';
  return n;
}

DLLEXPORT size_t pdf_write_comment(PDF_TYPE_STRING val, size_t in_len, char* out, size_t out_len) {

  PDF_TYPE_STRING in_p = val;
  PDF_TYPE_STRING in_end = val + in_len;
  size_t n = 0;
  size_t m;

  n += (m = _bufcat(out+n, out_len-n, "% "));
  if (m == 0) return 0;

  while (in_p < in_end) {
    char ch = *(in_p++);
    if (ch == '\r' || ch == '\n') {
        if (ch == '\r' && in_p < in_end && *(in_p) == '\n') in_p++;
        n += (m = _bufcat(out+n, out_len-n, "% "));
        if (m == 0) return n;
    }
    else {
        out[n++] = ch;
    }
  }

  return n;
}

DLLEXPORT size_t pdf_write_xref_seg(PDF_TYPE_XREF xref, PDF_TYPE_UINT length, PDF_TYPE_STRING out, size_t out_len) {
  PDF_TYPE_UINT i;
  char entry[24];
  size_t n = 0, m;

  for (i = 0; i < length; i++) {
      uint64_t offset  = *(xref++);
      uint64_t gen_num = *(xref++);
      uint8_t type     = *(xref++) ? 'n' : 'f';

      sprintf(entry, "%010"PRIu64" %05" PRIu64" %c \n", offset, gen_num, type);
      n += (m = _bufcat(out+n, out_len-n, entry));
      if (m == 0) return 0;
  }

  return n;
}

DLLEXPORT int pdf_write_name_code(PDF_TYPE_CODE_POINT cp, char* out, size_t out_len) {
    uint8_t bp[5];
    uint8_t bytes;
    uint8_t i;
    char buf[4] = { '#', ' ', ' ', 0};
    size_t n = 0, m;

    if (out_len > 0 && cp >= (uint32_t) '!' && cp <= (uint32_t) '~') {
      // printable ascii character
      uint8_t c = (uint8_t) cp;
      switch (c) {
      case '#':
          /* '#' -> '##' */
          return _bufcat(out+n, out_len-n, "##");
      case '(': case ')':
      case '<': case '>':
      case '[': case ']':
      case '{': case '}':
      case '/': case '%':
          /* delimiter */
          break;
      default:
          out[n++] = c;
          return n;
      }
    }

    /* unicode escape */
    bytes = utf8_from_code(cp, bp);
    for (i = 0; i < bytes; i++) {
      uint8_t byte = bp[i];
      buf[1] = hex_char(byte / 16);
      buf[2] = hex_char(byte % 16);
      n += (m = _bufcat(out+n, out_len-n, buf));
      if (m == 0) return 0;
    }

    return n;
}

DLLEXPORT size_t pdf_write_name(PDF_TYPE_CODE_POINTS name, size_t in_len, char* out, size_t out_len) {

  PDF_TYPE_CODE_POINTS in_p = name;
  PDF_TYPE_CODE_POINTS in_end = name + in_len;
  size_t n = 0, m;

  if (n < out_len) out[n++] = '/';

  while (in_p < in_end) {
    uint32_t cp = *(in_p++);
    n += (m = pdf_write_name_code(cp, out+n, out_len-n));
    if (m == 0) return 0;
  }

  return n;
}
