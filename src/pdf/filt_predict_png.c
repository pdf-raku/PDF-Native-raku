#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "pdf.h"
#include "pdf/filt_predict_png.h"
DLLEXPORT void
pdf_filt_predict_png_decode(uint8_t *buf,
                            uint8_t *out,
                            uint8_t colors,
                            uint8_t bpc,
                            uint16_t columns,
                            uint16_t rows
                            ) {
  int bit_mask = (1 << bpc) - 1;
  int row_size = colors * columns;
  int idx = 0;
  int n = 0;
  int row;
  uint8_t padding = 0;
  for (padding = 0; ((row_size + padding) * bpc) % 8; padding++) {
  }
  
  for (row = 0; row < rows; row++) {
    /* PNG prediction can vary from row to row */
    uint8_t tag = buf[idx++];
    int i;
    if (tag >= 10 && tag <= 14) {
      tag -= 10;
    }

    switch (tag) {
      case 0: { /* None */
        for (i = 0; i < row_size; i++) {
          out[n++] = buf[idx++];
        }
        break;
      }
      case 1: { /* Left */
        for (i = 0; i < colors; i++) {
          out[n++] = buf[idx++];
        }
        for (i = colors+1; i <= row_size; i++) {
          uint8_t left_val = out[n - colors];
          out[n++] = (buf[idx++] + left_val) & bit_mask;
        }
        break;
      }
      case 2: { /* Up */
        for (i = 0; i < row_size; i++) {
          uint8_t up_val = row ? out[n - row_size] : 0;
          out[n++] = (buf[idx++] + up_val) & bit_mask;
        }
        break;
      }
      case 3: { /* Average */
        for (i = 0; i < row_size; i++) {
          uint8_t left_val = i < colors ? 0 : out[n - colors];
          uint8_t up_val = row ? out[n - row_size] : 0;
          out[n++] = (buf[idx++] + ((left_val + up_val) / 2 )) & bit_mask;
          }
        break;
      }
      case 4: { /* Paeth */
        for (i = 0; i < row_size; i++) {
          int left_val = i < colors ? 0 : out[n - colors];
          int up_val = row ? out[n - row_size] : 0;
          int up_left_val = row && i >= colors ? out[n - colors - row_size] : 0;

          int p = left_val + up_val - up_left_val;
          int pa = abs(p - left_val);
          int pb = abs(p - up_val);
          int pc = abs(p - up_left_val);
          int nearest =  pa <= pb && pa <= pc
            ? left_val
            : (pb <= pc ? up_val : up_left_val);

          out[n++] = (buf[idx++] + nearest) & bit_mask;
        }
        break;
      }
    default: {
        fprintf(stderr, "bad PNG predictor tag: %d\n", tag);
      }
    }

    idx += padding;
  }
}

DLLEXPORT void pdf_filt_predict_png_encode(uint8_t *buf,
                                        uint8_t *out,
                                        uint8_t colors,
                                        uint8_t bpc,
                                        uint16_t columns,
                                        uint16_t rows,
                                        uint8_t tag
                                        ) {
  int bit_mask = (1 << bpc) - 1;
  int row_size = colors * columns;
  int idx = 0;
  int n = 0;
  int row;
  uint8_t padding = 0;
  for (padding = 0; ((row_size + padding) * bpc) % 8; padding++) {
  }

  if (tag == 15) {
    /* optimize - just use Paeth */
    tag = 4;
  }
  else if (tag >= 10 && tag <= 14) {
      tag -= 10;
  }
  if (tag > 5) {
    fprintf(stderr, "bad PNG predictor tag: %d\n", tag);
    return;
  }

  for (row = 0; row < rows; row++) {
    int i;

    out[n++] = tag;

    switch (tag) {
      case 0: { /* None */
        for (i = 0; i < row_size; i++) {
          out[n++] = buf[idx++];
        }
        break;
      }
      case 1: { /* Left */
        for (i = 0; i < colors; i++) {
          out[n++] = buf[idx++];
        }
        for (i = colors+1; i <= row_size; i++) {
          uint8_t left_val = buf[idx - colors];
          out[n++] = (buf[idx++] - left_val) & bit_mask;
        }
        break;
      }
      case 2: { /* Up */
        for (i = 0; i < row_size; i++) {
          uint8_t up_val = row ? buf[idx - row_size] : 0;
          out[n++] = (buf[idx++] - up_val) & bit_mask;
        }
        break;
      }
      case 3: { /* Average */
        for (i = 0; i < row_size; i++) {
          uint8_t left_val = i < colors ? 0 : buf[idx - colors];
          uint8_t up_val = row ? buf[idx - row_size] : 0;
          out[n++] = (buf[idx++] - ((left_val + up_val) / 2 )) & bit_mask;
        }
        break;
      }
    case 4: { /* Paeth */
        for (i = 0; i < row_size; i++) {
          int left_val = i < colors ? 0 : buf[idx - colors];
          int up_val = row ? buf[idx - row_size] : 0;
          int up_left_val = row && i >= colors ? buf[idx - colors - row_size] : 0;

          int p = left_val + up_val - up_left_val;
          int pa = abs(p - left_val);
          int pb = abs(p - up_val);
          int pc = abs(p - up_left_val);
          int nearest =  pa <= pb && pa <= pc
            ? left_val
            : (pb <= pc ? up_val : up_left_val);

          out[n++] = (buf[idx++] - nearest) & bit_mask;
        }
        break;
      }
    }

    for (i = 0; i < padding; i++) {
      out[n++] = 0;
    }
  }
}
