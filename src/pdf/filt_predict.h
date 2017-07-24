#ifndef PDF_FILT_PREDICT_H_
#define PDF_FILT_PREDICT_H_

typedef enum
{
  PDF_FILTER_NO_PREDICTION = 1,
  PDF_FILTER_TIFF_PREDICTOR = 2,
  PDF_FILTER_PNG_NONE_ALL_ROWS = 10,
  PDF_FILTER_PNG_SUB_ALL_ROWS = 11,
  PDF_FILTER_PNG_UP_ALL_ROWS = 12,
  PDF_FILTER_PNG_AVERAGE_ALL_ROWS = 13,
  PDF_FILTER_PNG_PAETH_ALL_ROWS = 14,
  PDF_FILTER_PNG_OPTIMUM = 15
} pdf_filt_predict_type_t;

// Decode None, TIFF or PNG predictors
DLLEXPORT void
pdf_filt_predict_decode(uint8_t *in,
                        uint8_t *out,
                        uint8_t predictor,
                        uint8_t colors,
                        uint8_t bpc,
                        uint16_t columns,
                        uint16_t rows );

// Encode None, TIFF or PNG predictors
DLLEXPORT void
pdf_filt_predict_encode(uint8_t *in,
                        uint8_t *out,
                        uint8_t predictor,
                        uint8_t colors,
                        uint8_t bpc,
                        uint16_t columns,
                        uint16_t rows );

#endif
