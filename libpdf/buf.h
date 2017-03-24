/* repack from m-byte to n-byte unsigned integers */
extern void pdf_buf_unpack_1(uint8_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_unpack_2(uint8_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_unpack_4(uint8_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_unpack_16(uint8_t *in, uint16_t *out, size_t in_len);
extern void pdf_buf_unpack_32(uint8_t *in, uint32_t *out, size_t in_len);

extern void pdf_buf_pack_1(uint8_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_pack_2(uint8_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_pack_4(uint8_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_pack_16(uint16_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_pack_32(uint32_t *in, uint8_t *out, size_t in_len);
// packing of /W variable length words; for example in XRef streams
extern void pdf_buf_pack_32_W(uint32_t *in, uint8_t *out, size_t in_len, uint8_t *w, size_t w_len);
extern void pdf_buf_unpack_32_W(uint8_t *in, uint32_t *out, size_t in_len, uint8_t *w, size_t w_len);
