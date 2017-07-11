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
  out;
}

DLLEXPORT uint8_t* pdf_write_int(PDF_INT val, uint8_t *out, size_t out_len) {
    uint8_t buf[32];
    snprintf(buf, sizeof(buf), "%d", val);
    strncpy(out, buf, out_len);
    if (out_len) out[out_len - 1] = 0;
    out;
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
    out;
}

