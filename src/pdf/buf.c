#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include "pdf.h"
#include "pdf/buf.h"

DLLEXPORT void pdf_buf_unpack_1(uint8_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; i++) {
    uint8_t v = in[i];
    uint8_t k;
    j += 8;
    for (k = 0; k < 8; k++) {
      out[j - k - 1] = v & 1;
      v >>= 1;
    }
  }
}
 
DLLEXPORT void pdf_buf_unpack_2(uint8_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; i++) {
    uint8_t v = in[i];
    uint8_t k;
    j += 4;
    for (k = 0; k < 4; k++) {
      out[j - k - 1] = v & 3;
      v >>= 2;
    }
  }
}
 
DLLEXPORT void pdf_buf_unpack_4(uint8_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; i++) {
    uint8_t v = in[i];
    out[j++] = v >> 4;
    out[j++] = v & 15;
  }
}
 
DLLEXPORT void pdf_buf_unpack_16(uint8_t *in, uint16_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; j++) {
    out[j] = in[i++] << 8;
    out[j] += in[i++];
  }
}

DLLEXPORT void pdf_buf_unpack_24(uint8_t *in, uint32_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; j++) {
    out[j] = in[i++] << 16;
    out[j] += in[i++] << 8;
    out[j] += in[i++];
  }
}
 
DLLEXPORT void pdf_buf_unpack_32(uint8_t *in, uint32_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; j++) {
    out[j]  = in[i++] << 24;
    out[j] += in[i++] << 16;
    out[j] += in[i++] << 8;
    out[j] += in[i++];
  }
}
 
DLLEXPORT void pdf_buf_pack_1(uint8_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; j++) {
    uint8_t k;
    out[j] = 0;
    for (k = 0; k < 8; k++) {
      out[j] <<= 1;
      out[j] += in[i++];
    }
  }
}
 
DLLEXPORT void pdf_buf_pack_2(uint8_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; j++) {
    int8_t k;
    out[j] = 0;
    for (k = 0; k < 4; k++) {
      out[j] <<= 2;
      out[j] += in[i++];
    }
  }
}
 
DLLEXPORT void pdf_buf_pack_4(uint8_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; j++) {
    out[j]  = in[i++] << 4;
    out[j] += in[i++];
  }
}
 
DLLEXPORT void pdf_buf_pack_16(uint16_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; i++) {
    uint16_t v = in[i];
    out[j++] = v >> 8;
    out[j++] = v;
  }
}
 
DLLEXPORT void pdf_buf_pack_24(uint32_t *in, uint8_t *out, size_t in_len) {
  size_t i;
  size_t j = 0;
  for (i = 0; i < in_len; i++) {
    uint32_t v = in[i];
    out[j++] = v >> 16;
    out[j++] = v >> 8;
    out[j++] = v;
  }
}

DLLEXPORT void pdf_buf_pack_32(uint32_t *in, uint8_t *out, size_t in_len) {
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

// compute /W for an array, return blocking factor
size_t pdf_buf_pack_compute_W_64(uint64_t *in, size_t in_len, uint8_t *w, size_t w_len) {
    size_t i;
    size_t bytes; // packed bytes per record
    for (i = 0; i < w_len; i++) {
        w[i] = 0;
    }
    // collect maximum sizes
    for (i = 0; i < in_len; i++) {
        uint8_t j = i % w_len;
        uint64_t v = in[i];
        for (;(v >> (w[j] * 8)); w[j]++) {
        }
    }
    for (i = 0, bytes = 0; i < w_len; i++) {
        bytes += w[i];
    }
    return bytes;
}

// packing of /W variable length words; for example in XRef streams
DLLEXPORT void pdf_buf_pack_W_64(uint64_t *in, uint8_t *out, size_t in_len, uint8_t *w, size_t w_len) {
  size_t i;
  int64_t j = -1;

  for (i = 0; i < in_len; i++) {
    uint64_t v = in[i];
    uint8_t n = w[i % w_len];
    uint8_t k;
    j += n;
    for (k = 0; k < n; k++) {
      out[j - k] = v;
      v >>= 8;
    }
  }
}

DLLEXPORT void pdf_buf_unpack_W_64(uint8_t *in, uint64_t *out, size_t in_len, uint8_t *w, size_t w_len) {
  size_t i;
  uint64_t j = 0;
  for (i = 0; i < in_len;) {
    uint64_t v = 0;
    uint8_t n = w[j % w_len];
    uint8_t k;

    for (k = 0; k < n; k++) {
      v <<= 8;
      v += in[i++];
    }
    out[j++] = v;
  }
}
