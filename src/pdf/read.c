#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "pdf.h"
#include "pdf/types.h"
#include "pdf/read.h"

DLLEXPORT size_t pdf_read_xref_seg(PDF_UINT64 *xref, PDF_UINT length, PDF_STRING buf, size_t buf_len) {
  PDF_STRING buf_p = buf;
  PDF_STRING buf_end = buf + buf_len;
  PDF_UINT i;

  for (i = 0; i < length; i++) {
    uint64_t offset;
    uint64_t gen_num;
    uint8_t   type;

      if ((buf_end - buf_p < 20)
          || (sscanf(buf_p, "%010ld %05ld %c", &offset, &gen_num, &type) != 3)
          || !(type == 'n' || type == 'f')) {
        break;
      }
      *(xref++) = offset;
      *(xref++) = gen_num;
      *(xref++) = (uint64_t) (type == 'n' ? 1 : 0);
      buf_p += 20;
  }

  return (size_t) i;
}

