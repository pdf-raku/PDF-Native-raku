#ifndef PDF_BUF_H_
#define PDF_BUF_H_

DLLEXPORT uint8_t* pdf_write_bool(PDF_BOOL val, uint8_t *out, size_t in_len);
DLLEXPORT uint8_t* pdf_write_int(PDF_INT val, uint8_t *out, size_t out_len);
DLLEXPORT uint8_t* pdf_write_real(PDF_REAL val, uint8_t *out, size_t in_len);
DLLEXPORT uint8_t* pdf_write_literal(PDF_STRING val, size_t in_len, PDF_STRING out, size_t out_len);
DLLEXPORT uint8_t* pdf_write_hex_string(PDF_STRING val, size_t in_len, PDF_STRING out, size_t out_len);
#endif
