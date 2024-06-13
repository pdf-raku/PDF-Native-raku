#ifndef PDF_FILT_PREDICT_PNG_H_
#define PDF_FILT_PREDICT_PNG_H_

// Decode PNG predictors
DLLEXPORT void
pdf_filt_predict_png_decode(uint8_t *buf,
                            uint8_t *out,
                            uint8_t colors,
                            uint8_t bpc,
                            uint16_t columns,
                            size_t rows
                            );

// Encode PNG predictors
DLLEXPORT void
pdf_filt_predict_png_encode(uint8_t *buf,
                            uint8_t *out,
                            uint8_t colors,
                            uint8_t bpc,
                            uint16_t columns,
                            size_t rows,
                            uint8_t predictor
                            );

#endif
