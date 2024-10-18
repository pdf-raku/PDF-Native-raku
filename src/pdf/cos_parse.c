/*
 * cos_parse.c:
 *
 * A small PDF object parser. It currently works within the scope of
 * singular objects.
 *
 * The primary goal is efficent parsing of COS objects, once they have
 * been identified and isolated by indexing or scanning a PDF file.
 *
 * There are three main functions:
 *
 * CosIndObj* cos_parse_ind_obj(char*, size_t, int)
 *   - parse an indirect object, format: <uint> <uint> <object> <endobj>
 *
 * CosNode* cos_parse_obj(char*, size_t);
 *   - parse an inner object, dictionary, arrays or other simple objects
 *
 * CosContent* cos_parse_content(char, size_t)
 *   - parse a content stream, as a series of CosOp* objects, sprinkled
 *     with occasional chunkier CosInlineImage* objects
 *
 */

#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/cos_parse.h"
#include "pdf/utf8.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>

typedef enum {
    COS_TK_START,
    COS_TK_DONE,
    COS_TK_DELIM,
    COS_TK_INT,
    COS_TK_REAL,
    COS_TK_NAME,
    COS_TK_WORD, /* None of the above */
} CosTkType;

typedef struct {
    CosTkType type;
    size_t pos;
    size_t len;
} CosTk;

typedef struct {
    char* buf;
    size_t buf_len;
    size_t buf_pos;
    CosTk* tk[3]; /* small look-ahead buffer */
    uint8_t n_tk;
} CosParserCtx;

static CosNode** _parse_objects(CosParserCtx*, size_t*, char*);

static size_t _skip_ws(CosParserCtx* ctx) {
    int in_comment = 0;

    for (; ctx->buf_pos < ctx->buf_len; ctx->buf_pos++) {
        unsigned char ch = ctx->buf[ctx->buf_pos];
        if (in_comment) {
            if (ch == '\n' || ch == '\r') in_comment = 0;
        }
        else if (ch == '%') {
            in_comment = 1;
        }
        else if (ch && !isspace(ch)) {
            break;
        }
    }

    return ctx->buf_pos;
}

// strict scan for crlf or lf. This may matter at a binary-data boundaries such
// as 'stream'..'endstream', or 'ID'..'EI'.
static int _scan_new_line(CosParserCtx* ctx, char eoln[]) {
    int len = 0;
    if (ctx->buf[ctx->buf_pos] == '\r' && ctx->buf_pos < ctx->buf_len) {
        eoln[len++] = ctx->buf[ctx->buf_pos++];
    }
    if (ctx->buf[ctx->buf_pos] == '\n' && ctx->buf_pos < ctx->buf_len) {
        eoln[len++] = ctx->buf[ctx->buf_pos++];
    }
    return len;
}

/* Outer scan for the next start token: */
/* - preceding comments and whitespace are skipped */
/* - names, numbers and words are returned as tokens */
/* - opening delimiter is returned for strings, arrays and dictionaries */
static CosTk* _scan_tk(CosParserCtx* ctx) {
    CosTk* tk;
    unsigned char prev_ch = ' ';
    int wb = 0;
    int got_digits = 0;

    assert(ctx->n_tk < 3);
    _skip_ws(ctx);

    tk = ctx->tk[ctx->n_tk++];
    tk->type = COS_TK_START;
    tk->pos  = ctx->buf_pos;
    tk->len  = 0;

    for (; ctx->buf_pos <= ctx->buf_len && !wb; ctx->buf_pos++) {

        if (ctx->buf_pos >= ctx->buf_len) { wb = 1; continue; }
        unsigned char ch = ctx->buf[ctx->buf_pos];

        if (ch >= '0' && ch <= '9') {
            got_digits = 1;
            switch (tk->type) {
            case COS_TK_START:
                tk->type = COS_TK_INT;
                break;
            case COS_TK_INT:
            case COS_TK_REAL:
            case COS_TK_NAME:
            case COS_TK_WORD:
                break;
            case COS_TK_DELIM:
            case COS_TK_DONE:
                wb = 1;
                break;
            }
        }
        else switch (ch) {
        case '+': case '-':
            switch (tk->type) {
            case COS_TK_START:
                tk->type = COS_TK_INT;
                break;
            case COS_TK_INT:
            case COS_TK_REAL:
                tk->type = COS_TK_WORD;
                break;
            case COS_TK_WORD:
            case COS_TK_NAME:
                break;
            case COS_TK_DONE:
            case COS_TK_DELIM:
                wb = 1;
                break;
            }
            break;
        case '.':
            switch (tk->type) {
            case COS_TK_START:
            case COS_TK_INT:
                tk->type = COS_TK_REAL;
                break;
            case COS_TK_REAL:
                tk->type = COS_TK_WORD;
                break;
            case COS_TK_WORD:
            case COS_TK_NAME:
                break;
            case COS_TK_DONE:
            case COS_TK_DELIM:
                wb = 1;
                break;
            }
            break;
        case '/':
            if (tk->type == COS_TK_START) {
                tk->type = COS_TK_NAME;
            }
            else {
                wb = 1;
            }
            break;
        case '<': case '>':
            if (tk->type == COS_TK_START) {
                tk->type = COS_TK_DELIM;
            }
            else if (!(tk->len == 1 && prev_ch == ch)) {
                /* allow just '<<' and '>>' as 2 character delimiters */
                wb = 1;
            }
            break;
        case '[': case ']':
        case '(': case ')':
        case '{': case '}':
            if (tk->type == COS_TK_START) {
                tk->type = COS_TK_DELIM;
            }
            else {
                wb = 1;
            }
            break;
        default:
            if (!ch || ch == '%' || isspace(ch)) {
                /* whitespace or starting comment */
                wb = 1;
            }
            else {
                /* misc character */
                switch (tk->type) {
                case COS_TK_NAME:
                    if (ch < '!' || ch > '~') tk->type = COS_TK_WORD;
                    break;
                case COS_TK_INT:
                case COS_TK_REAL:
                case COS_TK_START:
                    tk->type = COS_TK_WORD;
                    break;
                case COS_TK_WORD:
                    break;
                case COS_TK_DELIM:
                case COS_TK_DONE:
                    wb = 1;
                    break;
                }
            }
        }
        if (!wb) tk->len++;
        prev_ch = ch;
    }

    switch (tk->type) {
    case COS_TK_START:
        /* must be at end of input */
        tk->type = COS_TK_DONE;
        break;
    case COS_TK_INT:
    case COS_TK_REAL:
        if (!got_digits) {
            /* got just '+', '-', '.', but no actual digits */
            tk->type = COS_TK_WORD;
        }
        break;
    default:
        break;
    }

    ctx->buf_pos--;
    return tk;
}

static CosTk* _look_ahead(CosParserCtx* ctx, int n) {
    assert(n >= 1 && n <= 3);

    while (ctx->n_tk < n) {
        _scan_tk(ctx);
    }
    return ctx->tk[n - 1];
}

static void _resume_parse(CosParserCtx* ctx, void *pos) {
    assert(pos >= ctx->buf && pos <= ctx->buf + ctx->buf_len);
    ctx->n_tk = 0;
    ctx->buf_pos = (char*)pos - ctx->buf;
}

/* get current token and advance one token */
static CosTk* _shift(CosParserCtx* ctx) {
    CosTk* tk;
    assert(ctx->n_tk >= 1 && ctx->n_tk <= 3);
    tk = ctx->tk[0];
    ctx->tk[0] = ctx->tk[1];
    ctx->tk[1] = ctx->tk[2];
    ctx->tk[2] = tk;
    ctx->n_tk--;

    return tk;
}

static PDF_TYPE_INT _read_int(CosParserCtx* ctx, CosTk* tk) {
    PDF_TYPE_INT val = 0;
    size_t i = 0;
    int sign = 1;

    assert(tk->type == COS_TK_INT);

    switch (ctx->buf[tk->pos]) {
    case '-':
        sign = -1;
        i++;
        break;
    case '+':
        i++;
        break;
    }

    for (; i < tk->len; i++) {
        val *= 10;
        val += ctx->buf[tk->pos + i] - '0';
    }
    return sign > 0 ? val : -val;
}

static int _hex_value(char ch) {

    if (ch >= '0' && ch <= '9') {
        return ch - '0';
    }
    else if (ch >= 'A' && ch <= 'F') {
        return ch - 'A' + 10;
    }
    else if (ch >= 'a' && ch <= 'f') {
        return ch - 'a' + 10;
    }

    return -1;
}

static CosName* _parse_name(CosParserCtx* ctx) {
    CosTk* tk = _look_ahead(ctx, 1);
    CosName* name = NULL;

    if (tk->type == COS_TK_NAME) {
        uint8_t* bytes = malloc(tk->len + 5);
        char* pos = ctx->buf + tk->pos + 1; /* consume '/' */
        char* end = pos + tk->len - 1;
        size_t n_bytes;
        PDF_TYPE_CODE_POINTS codes = NULL;
        size_t n_codes;
        size_t i = 0;

        /* 1st pass: collect bytes */
        for (n_bytes = 0; pos < end; pos++) {
            unsigned char ch = *pos;

            if (ch == '#') {
                if (pos+1 >= end) goto bail;
                if (*(pos+1) == '#') {
                    bytes[n_bytes++] = *(++pos);
                }
                else if (pos + 1 > end) {
                    goto bail;
                }
                else {
                    int d1 = _hex_value(*(++pos));
                    int d2 = _hex_value(*(++pos));
                    if (d1 < 0 || d2 < 0) goto bail;

                    bytes[n_bytes++] = d1 * 16  +  d2;
                }
            }
            else {
                bytes[n_bytes++] = ch;
            }
        }

        /* 2nd pass: count codes */
        for (n_codes = 0, i = 0; i < n_bytes; n_codes++) {
            int char_len = utf8_char_len(bytes[i]);
            if (char_len <= 0) char_len = 1;
            i += char_len;
        }

        codes = malloc(n_codes * sizeof(PDF_TYPE_CODE_POINT));

        /* 3rd pass: assesemble codes */
        for (n_codes = 0, i = 0; i < n_bytes; n_codes++) {
            int char_len = utf8_char_len(bytes[i]);
            if (char_len <= 0) char_len = 1;
            codes[n_codes] = utf8_to_code(bytes + i);
            i += char_len;
        }

        name = cos_name_new(codes, n_codes);
    bail:
        free(bytes);
        if (codes) free(codes);
    }

    return name;
}

static char* _strnchr(char* buf, char c, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (*(buf + i) == c) return (buf + i);
    }
    return NULL;
}

static PDF_TYPE_REAL _read_real(CosParserCtx* ctx, CosTk* tk) {
    PDF_TYPE_REAL val = 0.0;
    PDF_TYPE_REAL frac = 0.0;
    PDF_TYPE_REAL magn = 1.0;
    char* buf = ctx->buf + tk->pos;
    char* end = buf + tk->len;
    char* dp  = _strnchr(buf, '.', tk->len);
    char* p;
    int sign = 1;

    assert(tk->type == COS_TK_REAL);

    switch (*buf) {
    case '-':
        sign = -1;
        buf++;
        break;
    case '+':
        buf++;
        break;
    }

    if (!dp) dp = end;

    while (*end == '0' && end > dp) end--;

    for (p = buf; p < dp; p++) {
        val *= 10;
        val += (*p - '0');
    }

    for (p = dp + 1; p < end; p++) {
        magn *= 10;
        frac *= 10;
        frac += (*p - '0');
    }

    val += frac / magn;
    return sign > 0 ? val : -val;
}

static int _at_token(CosParserCtx* ctx, CosTk* tk, char* word) {
    return tk->len == strlen(word) && strncmp(ctx->buf + tk->pos, word, tk->len) == 0;
}

static int _at_uint(CosParserCtx* ctx, CosTk* tk) {
    return tk->type == COS_TK_INT && isdigit(ctx->buf[tk->pos]);
}

static int _at_op(CosParserCtx* ctx, CosTk* tk) {
    if (tk->type == COS_TK_WORD) {
        size_t i;

        for (i = 0; i < tk->len; i++) {
            unsigned char ch = ctx->buf[tk->pos + i];
            if (!isgraph(ch)) return 0;
        }
        return 1;
    }
    return 0;
}

static int _shift_word(CosParserCtx* ctx, char* word) {
    CosTk* tk = _look_ahead(ctx, 1);
    int found = _at_token(ctx, tk, word);
    if (found) _shift(ctx);

    return found;
}

static void _done_objects(CosNode** objects, size_t n) {
    if (objects) {
        size_t i;
        for (i = 0; i < n; i++) {
            if (objects[i]) cos_node_done(objects[i]);
        }
        free(objects);
    }
}

static int _octal_nibble(char **pos, char *end, int val, int n) {
    char digit = *((*pos)++) - '0';
    val *= 8;
    val += digit;

    if (*pos < end && n < 3 && **pos >= '0' && **pos <= '7') {
        return _octal_nibble(pos, end, val, n + 1);
    }
    else {
        (*pos)--;
        return val;
    }
}

static int _lit_str_nibble(char **pos, char *end, int *nesting) {
    unsigned char ch = *(++(*pos));
    switch (ch) {
    case '\\': {
        if (*pos >= end) return -1;
        ch = *(++(*pos));
        if (ch >= '0' && ch <= '7') {
            return _octal_nibble(pos, end, 0, 1);
        }
        else switch (ch) {
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'b': return '\b';
        case 'f': return '\f';
        case '(': return '(';
        case ')': return ')';
        case '\\': return '\\';
        case '\r':
            if (*pos < end && *(*pos+1) == '\n') ++(*pos);
            /* fallthrough */
        case '\n':
            return _lit_str_nibble(pos, end, nesting);
        default:
            return **pos;
        }
    }
    case '(':
        (*nesting)++;
        return ch;
    case ')':
        return --*nesting <= 0 ? -1 : ch;
    default:
        return ch;
    }
}

static CosLiteralStr* _parse_lit_string(CosParserCtx* ctx) {
    CosLiteralStr* lit_string = NULL;
    char* lit_pos = ctx->buf + ctx->buf_pos - 1;
    char* lit_end = lit_pos;
    char* buf_end = ctx->buf + ctx->buf_len;
    size_t n_bytes = 0;
    int nesting = 1;

    /* count output bytes and locate the end of the string */
    while (_lit_str_nibble(&lit_end, buf_end, &nesting) >= 0) {
        n_bytes++;
    };

    if (nesting == 0) {
        /* found a properly terminated string; process it */
        PDF_TYPE_STRING bytes = malloc(n_bytes);
        int byte;
        nesting = 1;
        n_bytes = 0;

        while ((byte = _lit_str_nibble(&lit_pos, lit_end, &nesting)) >= 0) {
            bytes[n_bytes++] = byte;
        }

        lit_string = cos_literal_new(bytes, n_bytes);

        free(bytes);
    }

    _resume_parse(ctx, lit_end + 1);

    return lit_string;
}

static void _scan_ws(char** pos, char* end) {

    while (isspace(**pos) && *pos < end) {
        (*pos)++;
    }
}

static CosHexString* _parse_hex_string(CosParserCtx* ctx) {
    CosHexString* hex_string = NULL;
    char* hex_pos = ctx->buf + ctx->buf_pos;
    char* hex_end = _strnchr(hex_pos, '>', ctx->buf_len - ctx->buf_pos);

    if (hex_end) {
        size_t n = 0;
        size_t max_bytes = (hex_end - hex_pos + 1) / 2; /* two hex chars per byte   */
        PDF_TYPE_STRING hex_bytes = malloc(max_bytes);
        while (hex_pos < hex_end) {
            int d1, d2;
            _scan_ws(&hex_pos, hex_end);
            if (hex_pos >= hex_end) break;
            d1 = _hex_value(*(hex_pos++));

            _scan_ws(&hex_pos, hex_end);
            d2 = (hex_pos >= hex_end) ? 0 : _hex_value(*(hex_pos++));
            if (d1 < 0 || d2 < 0) goto bail;
            hex_bytes[n++] = d1 * 16  +  d2;
        }
        hex_string = cos_hex_string_new(hex_bytes, n);
    bail:
        free(hex_bytes);
        _resume_parse(ctx, hex_end + 1);
    }

    return hex_string;
}

static CosArray* _parse_array(CosParserCtx* ctx) {
    size_t n = 0;
    CosNode** objects = _parse_objects(ctx, &n, "]");
    CosArray* array = NULL;
    if (n == 0 || objects[n-1] != NULL) {
        array = cos_array_new(objects, n);
    }
    _done_objects(objects, n);
    return array;
}

static CosDict* _pairs_to_dict(CosNode** objects, size_t n) {
    size_t elems = n / 2;
    CosDict* dict = NULL;
    CosName** keys = NULL;
    CosNode** values = NULL;
    size_t i;

    if (n % 2) goto bail;
    if (n && (!objects || objects[n-1] == NULL)) goto bail;
    keys  = malloc(elems * sizeof(CosName*));
    values = malloc(elems * sizeof(CosNode*));
    for (i = 0; i < elems; i ++) {
        CosName* key = (CosName*) objects[2*i];
        if (key->type != COS_NODE_NAME) goto bail;
        keys[i] = key;
        values[i] = objects[2*i + 1];
    }
    dict = cos_dict_new(keys, values, elems);
bail:
    _done_objects(objects, n);
    if (keys) free(keys);
    if (values) free(values);
    return dict;
}

static CosDict* _parse_dict(CosParserCtx* ctx) {
    size_t n = 0;
    CosNode** objects = _parse_objects(ctx, &n, ">>");
    return _pairs_to_dict(objects, n);
}

static CosNode* _parse_object(CosParserCtx* ctx) {
    CosNode* node = NULL;
    CosTk* tk1 = _look_ahead(ctx, 1);
    char ch = ctx->buf[tk1->pos];

    switch (tk1->type) {
    case COS_TK_INT:
        if (_at_uint(ctx, tk1) && _at_uint(ctx, _look_ahead(ctx, 2)) && _at_token(ctx, _look_ahead(ctx, 3), "R")) {
            /* indirect object <uint> <uint> R */
            uint64_t obj_num = _read_int(ctx, _shift(ctx));
            uint32_t gen_num = _read_int(ctx, _shift(ctx));
            node = (CosNode*)cos_ref_new(obj_num, gen_num);
        }
        else {
            /* continue with simple integer */
            PDF_TYPE_INT val = _read_int(ctx, tk1);
            node = (CosNode*)cos_int_new(val);
        }
        break;
    case COS_TK_NAME:
        node = (CosNode*) _parse_name(ctx);
        break;
    case COS_TK_REAL: {
        PDF_TYPE_REAL val = _read_real(ctx, tk1);
        node = (CosNode*)cos_real_new(val);
        break;
    }
    case COS_TK_WORD:
        switch (tk1->len) {
        case 4:
            if (_at_token(ctx, tk1, "true")) {
                node = (CosNode*)cos_bool_new(1);
            }
            else if (_at_token(ctx, tk1, "null")) {
                node = (CosNode*)cos_null_new();
            }
            break;
        case 5:
            if (_at_token(ctx, tk1, "false")) {
                node = (CosNode*)cos_bool_new(0);
            }
            break;
        }
        /* if (!node) {
            fprintf(stderr, "ignoring word at position %ld\n", tk1->pos);
            } */
        break;
    case COS_TK_DELIM:
        _shift(ctx);
        switch (ch) {
        case '[': { /* '[' array ']' */
            node = (void*) _parse_array(ctx);
            break;
        }
        case '(': { /* '(' string ')' */
            node = (void*) _parse_lit_string(ctx);
            break;
        }
        case '<': {
            switch (tk1->len) {
            case 1: /* '<' hex string '>' */
                node = (void*) _parse_hex_string(ctx);
                break;
            case 2: /* '<<' dictionary '>>' */
                node = (void*) _parse_dict(ctx);
                break;
            }
            break;
        }
        case ']': case '>':
        case '{': case '}':
            break;
        default:
            fprintf(stderr, "Unhandled delimiter: '%c'\n", ch);
            break;
        }
        break;
    case COS_TK_DONE:
        fprintf(stderr, "ended at position %" PRId64 "\n", ctx->buf_pos);
        break;
    default:
        fprintf(stderr, "todo parse token type %d at position %" PRId64 "\n", tk1->type, tk1->pos);
        break;
    }

    if (ctx->n_tk) _shift(ctx);

    return node;
}

static CosNode** _parse_objects(CosParserCtx* ctx, size_t* n, char *stopper) {
    CosNode** objects = NULL;
    CosTk* tk = _look_ahead(ctx, 1);

    if (_at_token(ctx, tk, stopper)) {
        /* stopper reached */
        if (n) {
            objects = malloc(*n * sizeof(CosNode*));
            memset(objects, 0, *n * sizeof(CosNode*));
        }
    }
    else {
        CosNode* object = _parse_object(ctx);
        size_t i = (*n)++;
        if (object) {
            /* success, continue parsing */
            objects = _parse_objects(ctx, n, stopper);
        }
        else {
            /* failure, stop parsing */
            objects = malloc(*n * sizeof(CosNode*));
            memset(objects, 0, *n * sizeof(CosNode*));
        }
        objects[i] = object;
    }

    return objects;
}

static int _valid_operand(CosNode* node) {
    if (node) {
        CosNodeType type = node->type;
        switch ((CosNodeType)type) {
        case COS_NODE_BOOL:
        case COS_NODE_HEX_STR:
        case COS_NODE_INT:
        case COS_NODE_LIT_STR:
        case COS_NODE_NAME:
        case COS_NODE_NULL:
        case COS_NODE_REAL:
        case COS_NODE_COMMENT:
            return 1;

        case COS_NODE_ARRAY:
        case COS_NODE_DICT: {
            struct CosArrayishNode* a = (void*) node;
            size_t i;
            for (i = 0; i < a->elems; i++) {
                if (! _valid_operand( a->values[i]) ) return 0;
            }
            return 1;
        }

        case COS_NODE_ANY:
        case COS_NODE_CONTENT:
        case COS_NODE_IND_OBJ:
        case COS_NODE_INLINE_IMAGE:
        case COS_NODE_OP:
        case COS_NODE_REF:
        case COS_NODE_STREAM:
            break;
        }
    }
    return 0;
}

/* parse one operation, (operands followed by an operator), from a content stream */
static CosOp* _parse_content_operation(CosParserCtx* ctx, size_t* m) {
    CosOp* op = NULL;
    CosTk* tk = _look_ahead(ctx, 1);

    switch(tk->type) {
    case COS_TK_WORD:
        if (_at_op(ctx, tk)) {
            op = cos_op_new(ctx->buf + tk->pos, tk->len, NULL, *m);
            _shift(ctx);
        }
        break;
    case COS_TK_DONE:
        break;
    default: {
        CosNode* operand = _parse_object(ctx);
        size_t i = (*m)++;

        if (operand && _valid_operand(operand)) {
            /* success, continue parsing */
            op = _parse_content_operation(ctx, m);
        }

        if (op) {
            op->values[i] = operand;
        }
        else {
            cos_node_done(operand);
        }
    }
    }

    return op;
}

/* ID should follow a BI (begin image) operation, it:
   - has preceding /name <value> pairs as arguments
   - may have a /L n or /Length n argument for image data length
   - is followed by an image-data stream,
   - terminated by 'EI' (end image operator)
*/
static CosInlineImage* _parse_inline_image(CosParserCtx* ctx) {
    size_t n = 0;
    CosNode** objects = _parse_objects(ctx, &n, "ID");
    CosInlineImage* inline_image = NULL;

    if (objects) {
        unsigned char* start_image = (unsigned char*) ctx->buf + ctx->buf_pos;
        int ok = isspace(*(start_image++));
        CosDict* dict = NULL;
        size_t image_len = 0;

        if (ok) {
            dict = _pairs_to_dict(objects, n);
            if (! _valid_operand((CosNode*)dict)) {
                cos_node_done((CosNode*)dict);
                return NULL;
            }
        }

        if (ok && dict) {
            /* PDF 2.0 Mandates a /L or /Length entry to determine image length */
            static PDF_TYPE_CODE_POINT Length[6] = {'L', 'e', 'n', 'g', 't', 'h'};
            CosName* len_entry = cos_name_new(Length, 6);
            CosInt* len_lookup = (CosInt*) cos_dict_lookup(dict, len_entry);
            if (!len_lookup) {
                /* /Length not present, try /L */
                len_entry->value_len = 1;
                len_lookup = (CosInt*) cos_dict_lookup(dict, len_entry);
            }
            cos_node_done((CosNode*)len_entry);

            if (len_lookup) {
                /* content length supplied in the dictionary */
                if (len_lookup->type == COS_NODE_INT && len_lookup->value >= 0
                    && ctx->buf_pos + len_lookup->value < ctx->buf_len - 3 /* can fit <value> + " EI" */
                    ) {
                    image_len = len_lookup->value;
                    ctx->buf_pos += image_len;
                }
                else ok = 0;
            }
            else {
                /* we need to (gulp) scan for the end of image data */
                /* look for terminating <ws>EI</b> */
                unsigned char* p;
                unsigned char* end_image = NULL;
                unsigned char* end = (unsigned char*) ctx->buf + ctx->buf_len - 2;

                for (p = start_image; p < end && !end_image; p++) {
                    if (isspace(p[0]) && p[1] == 'E' && p[2] == 'I') {
                        /* Confirm we can actually parse 'EI' as a word */
                        _resume_parse(ctx,  p);
                        if (_at_token(ctx, _look_ahead(ctx, 1), "EI")) {
                            end_image = p;
                        }
                    }
                }

                if (end_image) {
                    image_len = end_image - start_image;
                }
                else {
                    ok = 0;
                }
            }
        }

        if (ok) {
            inline_image = cos_inline_image_new(dict, start_image, image_len);
        }

        /* restart parse just before "EI" */
        _resume_parse(ctx, start_image + image_len);
    }

    return inline_image;
}

/* parse content as a series of operations */
static CosContent* _parse_content(CosParserCtx* ctx, size_t* n, int expect_inline) {
    CosContent* content = NULL;
    CosTk* tk = _look_ahead(ctx, 1);
    if (tk->type == COS_TK_DONE) {
        if (!expect_inline) {
            content = cos_content_new(NULL, *n);
        }
    }
    else {
        size_t args = 0;
        CosOp* opn = expect_inline ? (CosOp*) _parse_inline_image(ctx) : _parse_content_operation(ctx, &args);
        if (opn) {
            expect_inline = !expect_inline && opn->sub_type == COS_OP_BeginImage;
            size_t i = (*n)++;
            content = _parse_content(ctx, n, expect_inline);

            if (content) {
                content->values[i] = opn;
            }
            else {
                cos_node_done((CosNode*)opn);
            }
        }
    }

    return content;
}

/* locate 'endstream' at the end of stream data, working from the end of
 * the buffer backwards. Attempt to match the same end-of-line sequence to
 * avoid accidentally consuming binary data.
 */
static size_t _locate_endstream(CosParserCtx* ctx, size_t start, char eoln[]) {
    size_t p;
    int len = strlen(eoln);
    int guess = len == 0;

    for (p = ctx->buf_len - 10; p > start; p--) {
        if (strncmp("endstream", ctx->buf + p, 9) == 0) {
            if (guess || strncmp(eoln, ctx->buf + p - len, len) == 0) {
                if (guess)  {
                    if (p > start   && (ctx->buf[p-1] == '\n' || ctx->buf[p-1] == '\r')) len++;
                    if (p > start+1 && (ctx->buf[p-2] == '\n' || ctx->buf[p-2] == '\r')) len++;
                }
                return p - len;
            }
        }
    }
    return 0;
}

DLLEXPORT CosIndObj* cos_parse_ind_obj(char* in_buf, size_t in_len, CosParseMode mode) {
    CosTk tk1 = {COS_TK_START, 0, 0}, tk2 = {COS_TK_START, 0, 0}, tk3 = {COS_TK_START, 0, 0};
    CosParserCtx _ctx = { in_buf, in_len, 0, {&tk1, &tk2, &tk3}, 0};
    CosParserCtx* ctx = &_ctx;
    CosIndObj* ind_obj = NULL;

    _look_ahead(ctx, 3); /* load tokens: tk1, tk2, tk3 */

    if (_at_uint(ctx, &tk1) && _at_uint(ctx, &tk2) && _at_token(ctx, &tk3, "obj") ) {
        /* indirect object header: <uint> <uint> obj */
        uint64_t obj_num = _read_int(ctx, _shift(ctx));
        uint32_t gen_num = _read_int(ctx, _shift(ctx));
        CosNode* object;

        _shift(ctx); /* skip 'obj' keyword */
        object = _parse_object(ctx);

        if (object && object->type == COS_NODE_DICT) {
            /* possible upgrade of dict to a stream */
            CosDict* dict = (void*)object;
            char eoln[3] = {0, 0, 0 };
            char guess_eoln[1] = {0};

            if (_shift_word(ctx, "stream") && _scan_new_line(ctx, eoln)) {
                CosStream* stream = NULL;
                size_t stream_start = ctx->buf_pos;

                if (mode == COS_PARSE_NIBBLE) {
                    stream = cos_stream_new(dict, NULL, stream_start);
                }
                else {
                    /* Eager parsing of stream data */
                    uint8_t *value = NULL;
                    size_t length = 0;
                    size_t stream_end = _locate_endstream(ctx, stream_start, eoln);
                    if (!stream_end) stream_end = _locate_endstream(ctx, stream_start, guess_eoln);
                    if (stream_end) {
                        value = (uint8_t*) ctx->buf + stream_start;
                        length = stream_end - stream_start;

                        stream = cos_stream_new(dict, value, length);

                        _resume_parse(ctx, value + length);
                        _shift_word(ctx, "endstream");
                    }
                }

                if (!stream) cos_node_done((CosNode*)dict);
                object = (void*) stream;
            }
        }
        if (object) {
            if ((object->type == COS_NODE_STREAM && mode == COS_PARSE_NIBBLE) || _shift_word(ctx, "endobj")) {
                ind_obj = cos_ind_obj_new(obj_num, gen_num, object);
            }
            else {
                cos_node_done(object);
            }
        }
    }

    return ind_obj;
}

DLLEXPORT CosNode* cos_parse_obj(char* in_buf, size_t in_len) {
    CosTk tk1 = {COS_TK_START, 0, 0}, tk2 = {COS_TK_START, 0, 0}, tk3 = {COS_TK_START, 0, 0};
    CosParserCtx ctx = { in_buf, in_len, 0, {&tk1, &tk2, &tk3}, 0};
    return _parse_object(&ctx);
}

DLLEXPORT CosContent* cos_parse_content(char* in_buf, size_t in_len) {
    CosTk tk1 = {COS_TK_START, 0, 0}, tk2 = {COS_TK_START, 0, 0}, tk3 = {COS_TK_START, 0, 0};
    CosParserCtx ctx = { in_buf, in_len, 0, {&tk1, &tk2, &tk3}, 0};
    size_t n = 0;
    return _parse_content(&ctx, &n, 0);
}
