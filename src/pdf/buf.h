#ifndef PDF_BUF_H_
#define PDF_BUF_H_

// unpack bytes to n-bit unsigned integers
DLLEXPORT void pdf_buf_unpack_1(uint8_t *in, uint8_t *out, size_t in_len);
DLLEXPORT void pdf_buf_unpack_2(uint8_t *in, uint8_t *out, size_t in_len);
DLLEXPORT void pdf_buf_unpack_4(uint8_t *in, uint8_t *out, size_t in_len);
DLLEXPORT void pdf_buf_unpack_16(uint8_t *in, uint16_t *out, size_t in_len);
DLLEXPORT void pdf_buf_unpack_32(uint8_t *in, uint32_t *out, size_t in_len);

// pack from n-bit unsigned integers to bytes
DLLEXPORT void pdf_buf_pack_1(uint8_t *in, uint8_t *out, size_t in_len);
DLLEXPORT void pdf_buf_pack_2(uint8_t *in, uint8_t *out, size_t in_len);
DLLEXPORT void pdf_buf_pack_4(uint8_t *in, uint8_t *out, size_t in_len);
DLLEXPORT void pdf_buf_pack_16(uint16_t *in, uint8_t *out, size_t in_len);
DLLEXPORT void pdf_buf_pack_32(uint32_t *in, uint8_t *out, size_t in_len);

// packing of /W variable length words; for example in XRef streams
DLLEXPORT void pdf_buf_pack_compute_W_64(uint64_t *in, size_t in_len, uint8_t *w, size_t w_len);
DLLEXPORT void pdf_buf_pack_W_64(uint64_t *in, uint8_t *out, size_t in_len, uint8_t *w, size_t w_len);
DLLEXPORT void pdf_buf_unpack_W_64(uint8_t *in, uint64_t *out, size_t in_len, uint8_t *w, size_t w_len);

DLLEXPORT uint64_t pdf_buf_pack_xref_stream(uint64_t *in, uint64_t *out, size_t in_rows, uint64_t *index, size_t *index_len);
DLLEXPORT size_t pdf_buf_unpack_xref_stream(uint64_t *in, uint64_t *out, size_t in_rows, uint64_t *index, size_t index_len);

#endif
