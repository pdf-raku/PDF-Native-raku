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

static int _bufcat(char* out, int out_len, char *in) {
    int n;
    for (n=0; in[n] && out_len > 0; n++) {
        out[n] = in[n];
    }
    return n;
}

DLLEXPORT size_t pdf_write_bool(PDF_TYPE_BOOL val, char *out, size_t out_len) {
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
  size_t n = 0;

  out_len--;

  if (n < out_len) out[n++] = '(';

  while (in_p < in_end && n < out_len) {
    char c = *(in_p++);
    char esc[3] = {'\\', 0 , 0};
    const char* sym;

    if (c && (sym = strchr("\n\r\t\f\b\0nrtfb", c))) {
      // symbolic escape
      esc[1] = sym[6];
      n += _bufcat(out+n, out_len-n, esc);
    }
    else {
      if (c && strchr("\\()", c)) {
          // escape: '\\', '(', ')'
          if (n+1 >= out_len) break;
          out[n++] = '\\';
      }
      if (n >= out_len) return 0;
      out[n++] = c;
    }
  }

  if (n <= out_len) out[n++] = ')';
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

  if (n < out_len) out[n++] = '<';

  while (n < out_len - 2 && in_p < in_end) {
    uint8_t c = *(in_p++);
    if (n >= out_len - 1) return 0;
    out[n++] = hex_char(c / 16);
    out[n++] = hex_char(c % 16);
  }

  if (n >= out_len) return 0;
  out[n++] = '>';
  return n;
}

DLLEXPORT size_t pdf_write_xref_seg(PDF_TYPE_XREF xref, PDF_TYPE_UINT length, PDF_TYPE_STRING out, size_t out_len) {
  PDF_TYPE_UINT i;
  char entry[24];
  size_t n = 0;

  for (i = 0; i < length; i++) {
      uint64_t offset  = *(xref++);
      uint64_t gen_num = *(xref++);
      uint8_t type     = *(xref++) ? 'n' : 'f';

      sprintf(entry, "%010"PRIu64" %05" PRIu64" %c \n", offset, gen_num, type);
      n += _bufcat(out+n, out_len-n, entry);
      if (n >= out_len) return 0;
  }

  return n;
}

static uint8_t utf8_encode(uint8_t *bp, uint32_t cp) {
    if (cp <= 0x7F) {
        bp[0] = (uint8_t)cp;
        return 1;
    }

    if (cp <= 0x07FF) {
        bp[0] = (uint8_t)(( 6 << 5) |  (cp >> 6));
        bp[1] = (uint8_t)(( 2 << 6) |  (cp &  0x3F));
        return 2;
    }

    if (cp <= 0xFFFF) {
        bp[0] = (uint8_t)((14 << 4) |  (cp >> 12));
        bp[1] = (uint8_t)(( 2 << 6) | ((cp >> 6) & 0x3F));
        bp[2] = (uint8_t)(( 2 << 6) | ( cp       & 0x3F));
        return 3;
    }

    if (cp <= 0x10FFFF) {
        bp[0] = (uint8_t)((30 << 3) |  (cp >> 18));
        bp[1] = (uint8_t)(( 2 << 6) | ((cp >> 12) & 0x3F));
        bp[2] = (uint8_t)(( 2 << 6) | ((cp >>  6) & 0x3F));
        bp[3] = (uint8_t)(( 2 << 6) | ( cp        & 0x3F));
        return 4;
    }

    return 0;
}

DLLEXPORT size_t pdf_write_name(PDF_TYPE_CODE_POINTS name, size_t in_len, char* out, size_t out_len) {

  PDF_TYPE_CODE_POINTS in_p = name;
  PDF_TYPE_CODE_POINTS in_end = name + in_len;
  size_t n = 0;

  if (n < out_len) out[n++] = '/';

  while (in_p < in_end) {
    uint32_t cp = *(in_p++);
    uint8_t bp[5];
    uint8_t m;
    uint8_t i;
    uint8_t byte;
    char buf[4] = { '#', ' ', ' ', 0};

    if (cp >= (uint32_t) '!' && cp <= (uint32_t) '~') {
      // regular printable ascii character
      uint8_t c = (uint8_t) cp;
      if (c == '#') {
        n += _bufcat(out+n, out_len-n, "##");
        continue;
      }
      else if (!strchr("()<>[]{}/%", c)) {
          if (n < out_len) out[n++] = c;
          continue;
      }
    }
    m = utf8_encode(bp, cp);
    for (i = 0; i < m; i++) {
      byte = bp[i];
      buf[1] = hex_char(byte / 16);
      buf[2] = hex_char(byte % 16);
      n += _bufcat(out+n, out_len-n, buf);
    }

    if (n >= out_len) return 0;
  }

  return n;
}
