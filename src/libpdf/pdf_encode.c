/* Get prototype. */
#include <stddef.h>
#include <stdint.h>
#include <libpdf/pdf_encode.h>
static const char b64c[64] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void
base64_encode_blocks (uint8_t *in, size_t block_count, uint8_t *out) {
  for (;block_count > 0; block_count--, in += 3)
    {
      *out++ = b64c[in[0] >> 2];
      *out++ = b64c[((in[0] << 4) + (in[1] >> 4)) & 0x3f];
      *out++ = b64c[((in[1] << 2) + (in[2] >> 6)) & 0x3f];
      *out++ = b64c[in[2] & 0x3f];
    }
}

void
pdf_encode_base64 (uint8_t* in, size_t inlen,
		uint8_t* out, size_t outlen)
{
  size_t whole_blocks = inlen / 3;
  if (whole_blocks * 4 > outlen) {
    whole_blocks = outlen / 4;
  }
  base64_encode_blocks (in, whole_blocks, out);
  inlen -= whole_blocks * 3;
  outlen -= whole_blocks * 4;
  in += whole_blocks * 3;
  out += whole_blocks * 4;

  if (inlen && outlen) {
    /* Final partial block */
    uint8_t in_tail[3] = {
      in[0],
      inlen > 1 ? in[1] : 0,
      inlen > 2 ? in[2] : 0
    };
    uint8_t out_tail[4];
    uint8_t i;

    base64_encode_blocks (in_tail, 1, out_tail);
    /* Pad */
    if (inlen < 2) {
      out_tail[2] = '=';
    }
    out_tail[3] = '=';

    for (i = 0; i < 4 && outlen > 0; outlen--) {
      *out++ = out_tail[i++];
    }
  }

  if (outlen) {
    *out = '\0';
  }
}
