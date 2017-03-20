#include <stddef.h>
#include <stdint.h>
#include <libpdf/buf.h>

extern void pdf_buf_pack_8_4(uint8_t *out, uint8_t *in, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; i++) {
    uint8_t v = in[i];
    out[j++] = v >> 4;
    out[j++] = v & 15;
  }
}
 
