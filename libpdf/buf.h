/* repack from m-byte to n-byte unsigned integers
   note: 'out' buffer should be m*n*in_len bytes */
extern void pdf_buf_pack_8_4(uint8_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_pack_8_16(uint8_t *in, uint16_t *out, size_t in_len);
extern void pdf_buf_pack_8_32(uint8_t *in, uint32_t *out, size_t in_len);
extern void pdf_buf_pack_4_8(uint8_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_pack_16_8(uint16_t *in, uint8_t *out, size_t in_len);
extern void pdf_buf_pack_32_8(uint32_t *in, uint8_t *out, size_t in_len);
