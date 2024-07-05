#ifndef PDF_UTF8_H_
#define PDF_UTF8_H_

/*
  Adapted from: https://rosettacode.org/wiki/UTF-8_encode_and_decode#C
*/

typedef struct {
    char mask;    /* char data will be bitwise AND with this */
    char lead;    /* start bytes of current char in utf-8 encoded character */
    uint32_t beg; /* beginning of codepoint range */
    uint32_t end; /* end of codepoint range */
    int bits_stored; /* the number of bits from the codepoint that fits in char */
} utf_t;

static utf_t * utf[] = {
	/*             mask        lead        beg      end       bits */
	[0] = &(utf_t){0b00111111, 0b10000000, 0,       0,        6    },
	[1] = &(utf_t){0b01111111, 0b00000000, 0000,    0177,     7    },
	[2] = &(utf_t){0b00011111, 0b11000000, 0200,    03777,    5    },
	[3] = &(utf_t){0b00001111, 0b11100000, 04000,   0177777,  4    },
	[4] = &(utf_t){0b00000111, 0b11110000, 0200000, 04177777, 3    },
	      &(utf_t){0},
};

/* All lengths are in bytes */
int utf8_code_len(const uint32_t cp); /* len of associated utf-8 char */
int utf8_char_len(const char ch);          /* len of utf-8 encoded char */

int utf8_from_code(uint32_t cp, uint8_t* buf);
uint32_t utf8_to_code(const char chr[4]);

#endif
