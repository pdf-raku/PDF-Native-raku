#ifndef PDF_FILT_PREDICT_TIFF_H_
#define PDF_FILT_PREDICT_TIFF_H_

// Decode TIFF predictors
DLLEXPORT void
pdf_filt_predict_tiff_decode(uint8_t *in,
                               uint8_t *out,
                               uint8_t colors,
                               uint8_t bpc,
                               uint16_t columns,
                               uint16_t rows
                               );

// Encode TIFF predictors
DLLEXPORT void
pdf_filt_predict_tiff_encode(uint8_t *in,
                             uint8_t *out,
                             uint8_t colors,
                             uint8_t bpc,
                             uint16_t columns,
                             uint16_t rows
                             );

#endif
