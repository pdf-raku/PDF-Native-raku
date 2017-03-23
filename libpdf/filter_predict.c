#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <libpdf/filter_predict.h>

static void tiff_predict_8 (uint8_t* in,
                            uint8_t* out,
                            uint8_t colors,
                            uint16_t columns,
                            uint16_t rows) {
  int i;
  int j;
  int r;
  uint8_t* in_p = in;
  uint8_t* out_p = out;
  fprintf(stderr, "tiff-8: colors:%d columns:%d rows:%d\n", colors,columns,rows);
  for (r = 0; r < rows; r++) {
    uint8_t* prev = in_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++) {
        *out_p++ = *in_p++ - *prev++;
      }
    }
  }
}

static void tiff_predict_16 (uint8_t* in,
                             uint8_t* out,
                             uint8_t colors,
                             uint16_t columns,
                             uint16_t rows) {
  int i;
  int j;
  int r;
  uint16_t* in_p  = (uint16_t*) in;
  uint16_t* out_p = (uint16_t*) out;

  for (r = 0; r < rows; r++) {
    uint16_t* prev  = (uint16_t*) in;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++) {
          *out_p++ = *in_p++ - *prev++;
      }
    }
  }
}

static void tiff_predict_32 (uint8_t* in,
                             uint8_t* out,
                             uint8_t colors,
                             uint16_t columns,
                             uint16_t rows) {
  int i;
  int j;
  int r;
  uint32_t* in_p  = (uint32_t *)in;
  uint32_t* out_p = (uint32_t *)out;

  for (r = 0; r < rows; r++) {
    uint32_t* prev  = (uint32_t *)in_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++)
        {
          *out_p++ = *in_p++ - *prev++;
        }
    }
  }
}

static void tiff_predict(uint8_t *in,
                         uint8_t *out,
                         uint8_t colors,
                         uint8_t bpc,
                         uint16_t columns,
                         uint16_t rows
                         ) {
  if (bpc < 8) {
    bpc = 8;
  }
  switch (bpc) {
  case 8:
    tiff_predict_8(in, out, colors, columns, rows );
    break;
  case 16:
    tiff_predict_16(in, out, colors, columns, rows );
    break;
  case 32:
    tiff_predict_32(in, out, colors, columns, rows );
    break;
  default:
    memcpy(out, in, columns * colors * rows);
  }
}

extern void
pdf_filter_predict(
                   uint8_t *in,
                   uint8_t *out,
                   uint8_t predictor,
                   uint8_t colors,
                   uint8_t bpc,
                   uint16_t columns,
                   uint16_t rows
                   ) {
  switch (predictor) {
  case PDF_FILTER_TIFF_PREDICTOR:
    tiff_predict(in, out, colors, bpc, columns, rows);
    break;
  case PDF_FILTER_NO_PREDICTION:
  default :
    memcpy(out, in, columns * colors);
    break;
  }
}
