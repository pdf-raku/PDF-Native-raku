/* Get prototype. */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <libpdf/pdf_encode.h>
static const char b64_enc[64] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void
base64_encode_blocks (uint8_t *in, size_t block_count, uint8_t *out) {
  for (;block_count > 0; block_count--, in += 3)
    {
      *out++ = b64_enc[in[0] >> 2];
      *out++ = b64_enc[((in[0] << 4) + (in[1] >> 4)) & 0x3f];
      *out++ = b64_enc[((in[1] << 2) + (in[2] >> 6)) & 0x3f];
      *out++ = b64_enc[in[2] & 0x3f];
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

}

#define NotSetUp 253
#define WhiteSpace 254
#define NonDigit 255

static uint8_t b64_dec[256] = {NotSetUp};
static void build_b64_dec() {
  uint8_t i;
  for (i = 0; i <= 32; i++) {
    b64_dec[i] = WhiteSpace;
  }
  for (i = 33; i < 255; i++) {
    b64_dec[i] = NonDigit;
  }
  for (i = 0; i < 64; i++) {
    b64_dec[(uint8_t) b64_enc[i]] = i;
  }
  /* allow URI input characters */
  b64_dec[(uint8_t) '-'] = 62;
  b64_dec[(uint8_t) '_'] = 63;
}

uint8_t next_digit(uint8_t* in,
		   size_t inlen,
		   size_t *i,
		   uint8_t *n,
		   uint8_t *error
		   ) {
  uint8_t digit = 0;
  if (*i < inlen) {
    digit = b64_dec[ in[ (*i)++ ] ];
    if (digit == WhiteSpace) {
      digit = next_digit(in, inlen, i, n, error);
    }
    else {
      if (digit == NonDigit) {
	*error = 1;
	digit = next_digit(in, inlen, i, n, error);
      }
      else {
	(*n)++;
      }
    }
  }
  return digit;
}

int32_t pdf_decode_base64(uint8_t* in,
			  size_t inlen,
			  uint8_t* out,
			  size_t outlen) {

    size_t i;
    int32_t j;
    uint8_t error = 0;
    if (b64_dec[0] == NotSetUp) build_b64_dec();

    while (inlen > 0 && in[inlen - 1] == '='
	   || b64_dec[ in[inlen - 1] ] == WhiteSpace) {
      inlen--;
    }

    for (i = 0, j = 0; i < inlen && j < outlen && !error;) {

      uint8_t sextet[4] = {0, 0, 0, 0};
      uint32_t triple;
      uint8_t n_digits = 0;
      int8_t k, m;
      for (k = 0; k < 4; k++) {
	sextet[k] = next_digit(in, inlen, &i, &n_digits, &error);
      }

      triple = (sextet[0] << 3 * 6)
        + (sextet[1] << 2 * 6)
        + (sextet[2] << 1 * 6)
        + (sextet[3] << 0 * 6);

      m = n_digits == 4 ? 0 : n_digits == 3 ? 1 : 2;
      for (k = 2; k >= m && j < outlen; k--) {
	out[j++] = (triple >> k * 8) & 0xFF;
      }
    }

    return error ? -1 : j;
}
