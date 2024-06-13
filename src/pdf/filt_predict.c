#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "pdf.h"
#include "pdf/filt_predict.h"
#include "pdf/filt_predict_tiff.h"
#include "pdf/filt_predict_png.h"

DLLEXPORT void
pdf_filt_predict_decode(
                        uint8_t *in,
                        uint8_t *out,
                        uint8_t predictor,
                        uint8_t colors,
                        uint8_t bpc,
                        uint16_t columns,
                        size_t rows
                   ) {
  switch (predictor) {
  case PDF_FILTER_TIFF_PREDICTOR:
    pdf_filt_predict_tiff_decode(in, out, colors, bpc, columns, rows);
    break;
  case PDF_FILTER_PNG_NONE_ALL_ROWS:
  case PDF_FILTER_PNG_SUB_ALL_ROWS:
  case PDF_FILTER_PNG_UP_ALL_ROWS:
  case PDF_FILTER_PNG_AVERAGE_ALL_ROWS:
  case PDF_FILTER_PNG_PAETH_ALL_ROWS:
  case PDF_FILTER_PNG_OPTIMUM:
    pdf_filt_predict_png_decode(in, out, colors, bpc, columns, rows);
    break;
  case PDF_FILTER_NO_PREDICTION: {
    int w = bpc / 8;
    if (w < 1) {
      w = 1;
    }
    memcpy(out, in, colors * columns * rows * w);
    break;
  }
  default :
    fprintf(stderr, "%s: unknown predictor-type: %d\n", __FILE__, predictor);
    break;
  }
}

DLLEXPORT void
pdf_filt_predict_encode(
                        uint8_t *in,
                        uint8_t *out,
                        uint8_t predictor,
                        uint8_t colors,
                        uint8_t bpc,
                        uint16_t columns,
                        size_t rows
                   ) {
  switch (predictor) {
  case PDF_FILTER_TIFF_PREDICTOR:
    pdf_filt_predict_tiff_encode(in, out, colors, bpc, columns, rows);
    break;
  case PDF_FILTER_PNG_NONE_ALL_ROWS:
  case PDF_FILTER_PNG_SUB_ALL_ROWS:
  case PDF_FILTER_PNG_UP_ALL_ROWS:
  case PDF_FILTER_PNG_AVERAGE_ALL_ROWS:
  case PDF_FILTER_PNG_PAETH_ALL_ROWS:
  case PDF_FILTER_PNG_OPTIMUM:
    pdf_filt_predict_png_encode(in, out, colors, bpc, columns, rows, predictor);
    break;
  case PDF_FILTER_NO_PREDICTION: {
    int w = bpc / 8;
    if (w < 1) {
      w = 1;
    }
    memcpy(out, in, colors * columns * rows * w);
    break;
  }
  default :
    fprintf(stderr, "%s: unknown predictor-type: %d\n", __FILE__, predictor);
    break;
  }
}
