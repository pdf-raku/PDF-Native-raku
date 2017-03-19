/* repack from 8bit to 4byte unsigned integers
   note: 'out' buffer should be 2*in_len bytes */
extern void pdf_buf_pack_8_4(uint8_t *out, uint8_t *in, size_t in_len);
