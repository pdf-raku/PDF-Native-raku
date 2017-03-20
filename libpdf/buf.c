#include <stddef.h>
#include <stdint.h>
#include <libpdf/buf.h>

extern void pdf_buf_pack_8_4(uint8_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; i++) {
    uint8_t v = in[i];
    out[j++] = v >> 4;
    out[j++] = v & 15;
  }
}
 
extern void pdf_buf_pack_8_16(uint8_t *in, uint16_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; j++) {
    out[j]  = in[i++] << 8;
    out[j] += in[i++];
  }
}

extern void pdf_buf_pack_8_32(uint8_t *in, uint32_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; j++) {
    uint8_t v = in[i];
    out[j]  = in[i++] << 24;
    out[j] += in[i++] << 16;
    out[j] += in[i++] << 8;
    out[j] += in[i++];
  }
}
 
extern void pdf_buf_pack_4_8(uint8_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; j++) {
    out[j]  = in[i++] << 4;;
    out[j] += in[i++];
  }
}
 
extern void pdf_buf_pack_16_8(uint16_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; i++) {
    uint16_t v = in[i];
    out[j++] = v >> 8;
    out[j++] = v;
  }
}
 
extern void pdf_buf_pack_32_8(uint32_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; i++) {
    uint32_t v = in[i];
    out[j++] = v >> 24;
    out[j++] = v >> 16;
    out[j++] = v >> 8;
    out[j++] = v;
  }
}
 
