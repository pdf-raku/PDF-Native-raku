#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "pdf.h"
#include "pdf/types.h"
#include "pdf/write.h"
DLLEXPORT uint8_t* pdf_write_bool(PDF_BOOL val, uint8_t *out, size_t out_len) {
  strncpy(out,
          val ? "true" : "false",
          out_len);
  if (out_len) out[out_len - 1] = 0;
  return out;
}

DLLEXPORT uint8_t* pdf_write_int(PDF_INT val, uint8_t *out, size_t out_len) {
  uint8_t buf[32];
  snprintf(buf, sizeof(buf), "%d", val);
  strncpy(out, buf, out_len);
  if (out_len) out[out_len - 1] = 0;
  return out;
}

DLLEXPORT uint8_t* pdf_write_real(PDF_REAL val, uint8_t *out, size_t out_len) {
  uint8_t buf[32];
  uint8_t *t;
  char *dp;

  const char* fmt = (val > 99999 || val < -99999) ? "%.1f" : "%.6f";
  snprintf(buf, sizeof(buf), fmt, val);

  dp = strchr(buf, '.');
  if (dp) {
    /* delete an excessive decimal portion. */
    for (t = buf + strlen(buf) - 1
           ; t >= (uint8_t*) dp && (*t == '0' || *t == '.')
           ; t--) {
      *t = 0;
    }
  }

  strncpy(out, buf, out_len);
  if (out_len) out[out_len - 1] = 0;
  return out;
}

DLLEXPORT uint8_t* pdf_write_literal(PDF_STRING val, size_t in_len, PDF_STRING out, size_t out_len) {

  PDF_STRING in_p = val;
  PDF_STRING in_end = val + in_len;
  PDF_STRING out_p = out;
  PDF_STRING out_end = out + out_len;

  if (out_p < out_end) *(out_p++) = '(';

  while (in_p < in_end && out_p < out_end) {
    uint8_t c = *(in_p++);

    if (c < 32 || c > 126) {
      uint8_t esc[5];
      uint8_t* esc_p = esc;
      const char* sym;

      if (c && (sym = strchr("\n\r\t\f\b\0nrtfb", c))) {
        // symbolic escape
        esc[0] = '\\';
        esc[1] = sym[6];
        esc[2] = 0;
      }
      else {
        // octal escape
        uint8_t i;
        esc[4] = 0;
        for (i = 3; i > 0; i--) {
          esc[i] = (c % 8) + '0';
          c /= 8;
        }
        esc[0] = '\\';
      }
      while (*esc_p && out_p < out_end) {
        *(out_p++) = *(esc_p)++;
      }
    }
    else {
      if (strchr("\\%#/()<>[]{}", c)) {
        if (out_p < out_end) *(out_p++) = '\\';
      }

      if (out_p < out_end) *(out_p++) = c;
   }
  }

  if (out_p < out_end) *(out_p++) = ')';
  if (out_p < out_end) {
    *out_p = 0;
  }
  else {
    if (out_len) out[out_len-1] = 0;
  }

  return out;
}

static uint8_t hex_char(uint8_t c) {
  return ((c < 10)
          ? ('0' + c)
          : ('a' + (c - 10))
          );
}

DLLEXPORT uint8_t* pdf_write_hex_string(PDF_STRING val, size_t in_len, PDF_STRING out, size_t out_len) {

  PDF_STRING in_p = val;
  PDF_STRING in_end = val + in_len;
  PDF_STRING out_p = out;
  PDF_STRING out_end = out + out_len;

  if (out_p < out_end) *(out_p++) = '<';

  while (in_p < in_end && out_p < out_end - 1) {
    uint8_t c = *(in_p++);
    *(out_p++) = hex_char(c / 16);
    *(out_p++) = hex_char(c % 16);
  }

  if (out_p < out_end) *(out_p++) = '>';
  if (out_p < out_end) {
    *out_p = 0;
  }
  else {
    if (out_len) out[out_len-1] = 0;
  }

  return out;
}

