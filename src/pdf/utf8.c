/*
  Adapted from: https://rosettacode.org/wiki/UTF-8_encode_and_decode#C
*/
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pdf/utf8.h>

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

int utf8_code_len(const uint32_t cp)
{
    int len = 0;
    for(utf_t **u = utf; *u; ++u) {
        if((cp >= (*u)->beg) && (cp <= (*u)->end)) {
            break;
        }
        ++len;
    }
    if(len > 4) /* Out of bounds */
        exit(1);

    return len;
}

int utf8_char_len(const char ch)
{
    int len = 0;
    for (utf_t **u = utf; *u; ++u) {
        if((ch & ~(*u)->mask) == (*u)->lead) {
            break;
        }
        ++len;
    }
    if (len > 4) { /* Malformed leading byte */
        return -1;
    }
    return len;
}

int utf8_from_code(uint32_t cp, uint8_t* buf)
{
    const int bytes = utf8_code_len(cp);

    int shift = utf[0]->bits_stored * (bytes - 1);
    buf[0] = (cp >> shift & utf[bytes]->mask) | utf[bytes]->lead;
    shift -= utf[0]->bits_stored;
    for(int i = 1; i < bytes; ++i) {
        buf[i] = (cp >> shift & utf[0]->mask) | utf[0]->lead;
        shift -= utf[0]->bits_stored;
    }
    return bytes;
}

uint32_t utf8_to_code(const uint8_t chr[4])
{
    int bytes = utf8_char_len(*chr);
    int shift = utf[0]->bits_stored * (bytes - 1);
    uint32_t codep = (*chr++ & utf[bytes]->mask) << shift;

    for(int i = 1; i < bytes; ++i, ++chr) {
        shift -= utf[0]->bits_stored;
        codep |= ((char)*chr & utf[0]->mask) << shift;
    }

    return codep;
}
