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

static void _concat(char** out_p, char* end_p, char* buf) {
  uint32_t len = strlen(buf);
  if (len > end_p - *out_p) {
    len = end_p - *out_p;
  }
  memcpy(*out_p, buf, len);
  *out_p += len;
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
  size_t len;

  const char* fmt = (val > 9999999 || -val > 9999999) ? "%.1f" : "%.5f";
  snprintf(buf, sizeof(buf), fmt, val);

  dp = strchr(buf, '.');
  if (dp) {
    /* omit trailing zeros and maybe '.' */
    for (t = dp + strlen(dp) - 1
           ; t >= dp && (*t == '0' || t == dp)
           ; t--) {
    }
    len = t - buf + 1;
  }
  else {
      len = strnlen(buf, sizeof(buf));
  }

  if (len > out_len) len = out_len;
  memcpy(out, buf, len);
  return len;
}

DLLEXPORT size_t pdf_write_literal(PDF_TYPE_STRING val, size_t in_len, char* out, size_t out_len) {

  PDF_TYPE_STRING in_p = val;
  PDF_TYPE_STRING in_end = val + in_len;
  char* out_p = out;
  char* out_end = out + out_len - 1;

  if (out_p < out_end) *(out_p++) = '(';

  while (in_p < in_end && out_p < out_end) {
    char c = *(in_p++);
    char esc[3];
    const char* sym;

    if (c && (sym = strchr("\n\r\t\f\b\0nrtfb", c))) {
      // symbolic escape
      esc[0] = '\\';
      esc[1] = sym[6];
      esc[2] = 0;
      _concat(&out_p, out_end, esc);
    }
    else {
      if (c && strchr("\\()", c)) {
          // escape: '\\', '(', ')'
          if ((out_p+1) >= out_end) {
              break;
          }
          *(out_p++) = '\\';
      }

      if (out_p < out_end) *(out_p++) = c;
    }
  }

  if (out_p <= out_end) *(out_p++) = ')';
  return (size_t) (out_p - out);
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
  char* out_p = out;
  char* out_end = out + out_len - 1;

  if (out_p < out_end) *(out_p++) = '<';

  while (in_p < in_end && out_p < out_end - 1) {
    uint8_t c = *(in_p++);
    *(out_p++) = hex_char(c / 16);
    *(out_p++) = hex_char(c % 16);
  }

  if (out_p <= out_end) *(out_p++) = '>';
  return (size_t) (out_p - out);
}

DLLEXPORT size_t pdf_write_xref_seg(PDF_TYPE_XREF xref, PDF_TYPE_UINT length, PDF_TYPE_STRING buf, size_t buf_len) {
  PDF_TYPE_STRING buf_p = buf;
  PDF_TYPE_STRING buf_end = buf + buf_len;
  char entry[24];
  PDF_TYPE_UINT i;

  for (i = 0; i < length && buf_p < buf_end; i++) {
      uint64_t offset  = *(xref++);
      uint64_t gen_num = *(xref++);
      uint8_t type     = *(xref++) ? 'n' : 'f';

      sprintf(entry, "%010"PRIu64" %05" PRIu64" %c \n", offset, gen_num, type);
      _concat(&buf_p, buf_end, entry);
  }

  return (size_t) (buf_p - buf);
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
  char* out_p = out;
  char* out_end = out + out_len;

  if (out_p < out_end) *(out_p++) = '/';

  while (in_p < in_end && out_p < out_end - 1) {
    uint32_t cp = *(in_p++);
    uint8_t bp[5];
    uint8_t n;
    uint8_t i;
    uint8_t byte;
    char buf[4] = { '#', ' ', ' ', 0};

    if (cp >= (uint32_t) '!' && cp <= (uint32_t) '~') {
      // regular printable ascii character
      uint8_t c = (uint8_t) cp;
      if (c == '#') {
        _concat(&out_p, out_end, "##");
        continue;
      }
      else if (!strchr("()<>[]{}/%", c)) {
        if (out_p < out_end) *(out_p++) = c;
        continue;
      }
    }
    n = utf8_encode(bp, cp);
    for (i = 0; i < n; i++) {
      byte = bp[i];
      buf[1] = hex_char(byte / 16);
      buf[2] = hex_char(byte % 16);
      _concat(&out_p, out_end, buf);
    }
  }

  return (size_t) (out_p - out);
}
