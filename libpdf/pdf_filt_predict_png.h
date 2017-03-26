extern void
pdf_filt_predict_png_decode(uint8_t *in,
                              uint8_t *out,
                              uint8_t colors,
                              uint8_t bpc,
                              uint16_t columns,
                              uint16_t rows
                              );

extern void pdf_filt_predict_png_encode(uint8_t *in,
                                          uint8_t *out,
                                          uint8_t colors,
                                          uint8_t bpc,
                                          uint16_t columns,
                                          uint16_t rows,
                                          uint8_t predictor
                                          );
