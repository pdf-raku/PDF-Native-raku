#include <stdio.h>
#include <stdint.h>
#include <libpdf/pdf_filt_predict_tiff.h>
/* Decoding */

static void tiff_decode_nibble (uint8_t* in,
                               uint8_t* out,
                               uint8_t colors,
                               uint16_t columns,
                               uint16_t rows,
                               uint8_t bpc
                               ) {
  int i;
  int j;
  int r;
  uint8_t* in_p = in;
  uint8_t* out_p = out;
  uint8_t mask = (1 << bpc) - 1;

  for (r = 0; r < rows; r++) {
    uint8_t* prev = out_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++) {
        *out_p++ = ((*in_p++ & mask) + (*prev++ & mask)) & mask;
      }
    }
  }
}

static void tiff_decode_8 (uint8_t* in,
                            uint8_t* out,
                            uint8_t colors,
                            uint16_t columns,
                            uint16_t rows) {
  int i;
  int j;
  int r;
  uint8_t* in_p = in;
  uint8_t* out_p = out;

  for (r = 0; r < rows; r++) {
    uint8_t* prev = out_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++) {
        *out_p++ = *in_p++ + *prev++;
      }
    }
  }
}

static void tiff_decode_16 (uint8_t* in,
			    uint8_t* out,
			    uint8_t colors,
			    uint16_t columns,
			    uint16_t rows) {
  int i;
  int j;
  int r;
  uint16_t* in_p  = (uint16_t*) in;
  uint16_t* out_p = (uint16_t*) out;

  for (r = 0; r < rows; r++) {
    uint16_t* prev = out_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++) {
          *out_p++ = *in_p++ + *prev++;
      }
    }
  }
}

static void tiff_decode_32 (uint8_t* in,
			    uint8_t* out,
			    uint8_t colors,
			    uint16_t columns,
			    uint16_t rows) {
  int i;
  int j;
  int r;
  uint32_t* in_p  = (uint32_t *)in;
  uint32_t* out_p = (uint32_t *)out;

  for (r = 0; r < rows; r++) {
    uint32_t* prev = out_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++)
        {
          *out_p++ = *in_p++ + *prev++;
        }
    }
  }
}

extern void
pdf_filt_predict_tiff_decode(uint8_t *in,
			     uint8_t *out,
			     uint8_t colors,
			     uint8_t bpc,
			     uint16_t columns,
			     uint16_t rows
			     ) {
  switch (bpc) {
  case 1:
  case 2:
  case 4:
    tiff_decode_nibble(in, out, colors, columns, rows, bpc );
    break;
  case 8:
    tiff_decode_8(in, out, colors, columns, rows );
    break;
  case 16:
    tiff_decode_16(in, out, colors, columns, rows );
    break;
  case 32:
    tiff_decode_32(in, out, colors, columns, rows );
    break;
  default:
    fprintf(stderr, "%s: unhanded TIFF bpc: %d\n", __FILE__, bpc);
  }
}


/* Encoding */

static void tiff_encode_nibble (uint8_t* in,
				uint8_t* out,
				uint8_t colors,
				uint16_t columns,
				uint16_t rows,
				uint8_t bpc) {
  int i;
  int j;
  int r;
  uint8_t* in_p = in;
  uint8_t* out_p = out;
  uint8_t mask = (1 << bpc) - 1;

  for (r = 0; r < rows; r++) {
    uint8_t* prev = in_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++) {
        *out_p++ = ((*in_p++ & mask) - (*prev++ & mask)) & mask;
      }
    }
  }
}

static void tiff_encode_8 (uint8_t* in,
			   uint8_t* out,
			   uint8_t colors,
			   uint16_t columns,
			   uint16_t rows) {
  int i;
  int j;
  int r;
  uint8_t* in_p = in;
  uint8_t* out_p = out;

  for (r = 0; r < rows; r++) {
    uint8_t* prev = in_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++) {
        *out_p++ = *in_p++ - *prev++;
      }
    }
  }
}

static void tiff_encode_16 (uint8_t* in,
			    uint8_t* out,
			    uint8_t colors,
			    uint16_t columns,
			    uint16_t rows) {
  int i;
  int j;
  int r;
  uint16_t* in_p  = (uint16_t*) in;
  uint16_t* out_p = (uint16_t*) out;

  for (r = 0; r < rows; r++) {
    uint16_t* prev = in_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++) {
          *out_p++ = *in_p++ - *prev++;
      }
    }
  }
}

static void tiff_encode_32 (uint8_t* in,
			    uint8_t* out,
			    uint8_t colors,
			    uint16_t columns,
			    uint16_t rows) {
  int i;
  int j;
  int r;
  uint32_t* in_p  = (uint32_t *)in;
  uint32_t* out_p = (uint32_t *)out;

  for (r = 0; r < rows; r++) {
    uint32_t* prev = in_p;
    for (j = 0; j < colors; j++) {
      *out_p++ = *in_p++;
    }
    for (i = 1; i < columns; i++) {
      for (j = 0; j < colors; j++)
        {
          *out_p++ = *in_p++ - *prev++;
        }
    }
  }
}

extern void pdf_filt_predict_tiff_encode(uint8_t *in,
					 uint8_t *out,
					 uint8_t colors,
					 uint8_t bpc,
					 uint16_t columns,
					 uint16_t rows
					 ) {
  switch (bpc) {
  case 1:
  case 2:
  case 4:
    tiff_encode_nibble(in, out, colors, columns, rows, bpc );
    break;
  case 8:
    tiff_encode_8(in, out, colors, columns, rows );
    break;
  case 16:
    tiff_encode_16(in, out, colors, columns, rows );
    break;
  case 32:
    tiff_encode_32(in, out, colors, columns, rows );
    break;
  default:
    fprintf(stderr, "%s: unhanded TIFF bpc: %d\n", __FILE__, bpc);
  }
}
