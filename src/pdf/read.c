#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "pdf.h"
#include "pdf/types.h"
#include "pdf/read.h"

static uint8_t eoln_char(uint8_t c) {
  return (c == 10 || c == 13);
}

static uint8_t _line_length(PDF_STRING buf_start, PDF_STRING buf_end) {

  PDF_STRING buf_p = buf_start;
  int eoln = 0;

  while (buf_p <= buf_end) {
    if (eoln_char(buf_p[0])) {
      // end of line reached
      eoln++;
      if (buf_p < buf_end
          && eoln_char(buf_p[1])
          && buf_p[0] != buf_p[1]) {
        // consume \r\n or \n\r
        eoln++;
      }
      break;
    }
    buf_p++;
  }
  return buf_p - buf_start + eoln;
}

static uint8_t scan_num(PDF_STRING buf_p, uint8_t n, uint64_t *num) {
    uint8_t ok = 1;
    uint8_t j = 0;
    *num = 0;
    for (j=0; j < n; j++) {
      uint8_t d = buf_p[j];
      if (d >= '0' && d <= '9') {
        *num *= 10;
        *num += d - '0';
      }
      else {
        *num = 0;
        ok = 0;
        break;
      }
    }
    return ok;
}

static size_t skip_xref(PDF_STRING buf_p, PDF_STRING buf_end) {
  size_t bytes = 0;
  if ((buf_end - buf_p > 5)
      && strncmp(buf_p, "xref", 4)==0) {
    // skip 'xref' line
    bytes = _line_length(buf_p, buf_end);
  }
  return bytes;
}

/*
 * prescan index segments to find the number of entrys
 */
DLLEXPORT size_t pdf_read_xref_entry_count(PDF_STRING buf, size_t buf_len) {
  size_t entries = 0;
  PDF_STRING buf_p = buf;
  PDF_STRING buf_end = buf + buf_len;
  uint64_t obj_first_num;
  uint64_t obj_count;
  uint8_t line_len;

  buf_p += skip_xref(buf_p, buf_end);

  while ((buf_end - buf_p > 20)
         && (sscanf(buf_p, PRIu64 " " PRIu64, &obj_first_num, &obj_count) == 2)
         && (line_len = _line_length(buf_p, buf_end))) {
    entries += obj_count;
    buf_p += line_len + 20 * obj_count;
  }
  return entries;
}

DLLEXPORT size_t pdf_read_xref_seg(PDF_XREF xref, PDF_UINT length, PDF_STRING buf, size_t buf_len, size_t obj_first_num) {
  PDF_STRING buf_p = buf;
  PDF_STRING buf_end = buf + buf_len;
  PDF_UINT i;

  for (i = 0; i < length && (buf_end - buf_p >= 20); i++) {
    uint64_t offset;
    uint64_t gen_num;
    uint8_t  type = buf_p[17];

    if ((type == 'n' || type == 'f')  
        && scan_num(buf_p, 10, &offset)
        && scan_num(buf_p+11, 5, &gen_num)
        ) {
      *(xref++) = obj_first_num++;
      *(xref++) = (uint64_t) (type == 'n' ? 1 : 0);
      *(xref++) = offset;
      *(xref++) = gen_num;
      buf_p += 20;
    }
    else {
      // Error
      buf_p = buf;
      break;
    }
  }

  return (size_t) (buf_p - buf);
}

DLLEXPORT size_t pdf_read_xref(PDF_XREF xref, PDF_STRING buf, size_t buf_len) {
  size_t entries = 0;
  PDF_STRING buf_p = buf;
  PDF_STRING buf_end = buf + buf_len;
  uint64_t obj_first_num;
  uint64_t obj_count;
  uint8_t line_len;
  size_t n;

  buf_p += skip_xref(buf_p, buf_end);

  while ((buf_end - buf_p > 20)
         && (sscanf(buf_p, "%lld %lld", &obj_first_num, &obj_count) == 2)
         && (line_len = _line_length(buf_p, buf_end))) {
    buf_p += line_len;
    n = pdf_read_xref_seg(xref, obj_count, buf_p, buf_end - buf_p + 1, obj_first_num);
    if (n == 0 && obj_count != 0) {
      // error
      return 0;
    }
    entries += obj_count;
    buf_p += 20 * obj_count;
    xref += 4 * obj_count;
  }
  return (size_t) (buf_p - buf);
}
