#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/cos_parse.h"
#include "pdf/utf8.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

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

static size_t _skip_ws(CosParserCtx* ctx) {
    int in_comment = 0;

    for (; ctx->buf_pos < ctx->buf_len; ctx->buf_pos++) {
        char ch = ctx->buf[ctx->buf_pos];
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
// as 'stream'...'endstream', or 'ID'...'EI'.
static int _scan_new_line(CosParserCtx* ctx, int* dos_mode) {
    if (ctx->buf[ctx->buf_pos] == '\n' && ctx->buf_pos < ctx->buf_len) {
        ctx->buf_pos++;
        *dos_mode = 0;
        return 1;
    }
    else if (ctx->buf[ctx->buf_pos] == '\r' && ctx->buf_pos < ctx->buf_len-1 && ctx->buf[ctx->buf_pos+1] == '\n') {
        ctx->buf_pos += 2;
        *dos_mode = 1;
        return 1;
    }
    return 0;
}

/* Outer scan for the next start token: */
/* - preceding comments and whitespace are skipped */
/* - names, numbers, words and indirect references are returned as tokens */
/* - returns opening delimiter for strings, arrays and dictionaries */
static CosTk* _scan_tk(CosParserCtx* ctx) {
    CosTk* tk;
    char ch = ' ';
    char prev_ch = ' ';
    int wb = 0;
    int number = 0;
    int digits = 0;

    assert(ctx->n_tk < 3);

    tk = ctx->tk[ctx->n_tk++];
    _skip_ws(ctx);

    tk->type = COS_TK_START;
    tk->pos  = ctx->buf_pos;
    tk->len  = 0;

    for (; ctx->buf_pos < ctx->buf_len && !wb; ctx->buf_pos++) {
        ch = ctx->buf[ctx->buf_pos];
        tk->len++;
        switch (ch) {
        case '+': case '-':
            number = 1;
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
            number = 1;
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
        case '0'...'9':
            digits = 1;
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
            else if (!(tk->len == 2 && prev_ch == ch)) {
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
                    if (ch < '!' ||ch > '~') tk->type = COS_TK_WORD;
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
            break;
        }
        prev_ch = ch;
    }

    if (wb) {
        tk->len--;
    }
    else if (tk->type == COS_TK_START) {
        tk->type = COS_TK_DONE;
    }

    if (number && !digits) {
        /* got just '+', '-', '.', but no actual digits */
        tk->type = COS_TK_WORD;
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

/* get current token and advance to next */
static CosTk* _shift(CosParserCtx* ctx) {
    CosTk* tk;

    if (ctx->n_tk == 0) _look_ahead(ctx, 1);
    tk = ctx->tk[0];
    ctx->tk[0] = ctx->tk[1];
    ctx->tk[1] = ctx->tk[2];
    ctx->tk[2] = tk;
    ctx->n_tk--;

    return tk;
}

/* consume look-ahead buffer */
static void _advance(CosParserCtx* ctx) {
    ctx->n_tk = 0;
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

    switch (ch) {
        case '0'...'9':
            return ch - '0';
        case 'A'...'F':
            return ch - 'A' + 10;
        case 'a'...'f':
            return ch - 'a' + 10;
        default:
            return -1;
    }
}

static CosName* _read_name(CosParserCtx* ctx, CosTk* tk) {
    uint8_t* bytes = malloc(tk->len + 5);
    size_t n_bytes;
    PDF_TYPE_CODE_POINTS codes = NULL;
    size_t n_codes;
    size_t i = 0;
    char* pos = ctx->buf + tk->pos + 1; /* consume '/' */
    char* end = pos + tk->len - 1;
    CosName* name = NULL;

    assert(tk->type == COS_TK_NAME);

    /* 1st pass: collect bytes */
    for (n_bytes = 0; pos < end; pos++) {
        char ch = *pos;

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

    /* 3rd pass: produce codes */
    for (n_codes = 0, i = 0; i < n_bytes; n_codes++) {
        int char_len = utf8_char_len(bytes[i]);
        if (char_len <= 0) char_len = 1;
        codes[n_codes] = utf8_to_code(bytes + i);
        i += char_len;
    }

    name = cos_name_new(NULL, codes, n_codes);

bail:
    free(bytes);
    if (codes) free(codes);
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

   switch (*buf) {
    case '-':
        sign = -1;
        buf++;
        break;
    case '+':
        buf++;
        break;
    }

    assert(tk->type == COS_TK_REAL);
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
    return tk->type == COS_TK_INT && ctx->buf[tk->pos] != '-';
}

static int _get_token(CosParserCtx* ctx, char* word) {
    int found = 0;
    CosTk* tk = _look_ahead(ctx, 1);
    found = _at_token(ctx, tk, word);
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
        switch (*(++(*pos))) {
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'b': return '\b';
        case 'f': return '\f';
        case '(': return '(';
        case ')': return ')';
        case '\\': return '\\';
        case '0' ... '7':
            return _octal_nibble(pos, end, 0, 1);
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

    while (_lit_str_nibble(&lit_end, buf_end, &nesting) >= 0) {
        /* count output bytes and locate the end of the string */
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

        lit_string = cos_literal_new(NULL, bytes, n_bytes);

        free(bytes);
    }

    ctx->buf_pos = lit_end - ctx->buf + 1;

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
        hex_string = cos_hex_string_new(NULL, hex_bytes, n);
    bail:
        free(hex_bytes);
        ctx->buf_pos = hex_end - ctx->buf + 1;
    }

    return hex_string;
}

static CosNode** _parse_objects(CosParserCtx*, size_t*, char*);

static CosArray* _parse_array(CosParserCtx* ctx) {
    size_t n = 0;
    CosArray* array = NULL;
    CosNode** objects = _parse_objects(ctx, &n, "]");
    if (n == 0 || objects[n-1] != NULL) {
        array = cos_array_new(NULL, objects, n);
    }
    _done_objects(objects, n);
    return array;
}

static CosDict* _parse_dict(CosParserCtx* ctx) {
    size_t n = 0, i;
    CosDict* dict = NULL;
    CosNode** objects = _parse_objects(ctx, &n, ">>");
    size_t elems = n / 2;
    CosName** keys = NULL;
    CosNode** values = NULL;

    if (n % 2 || (objects && objects[n-1] == NULL)) goto bail;
    keys  = malloc(elems * sizeof(CosName*));
    values = malloc(elems * sizeof(CosNode*));
    for (i = 0; i < elems; i ++) {
        CosName* key = (CosName*) objects[2*i];
        if (key->type != COS_NODE_NAME) goto bail;
        keys[i] = key;
        values[i] = objects[2*i + 1];
    }
    dict = cos_dict_new(NULL, keys, values, elems);
bail:
    _done_objects(objects, n);
    if (keys) free(keys);
    if (values) free(values);

    return dict;
}

static CosNode* _parse_object(CosParserCtx* ctx) {
    CosNode* node = NULL;
    CosTk* tk1 = _look_ahead(ctx, 1);
    char ch = ctx->buf[tk1->pos];

    switch (tk1->type) {
    case COS_TK_INT:
        if (_at_uint(ctx, tk1) && _at_uint(ctx, _look_ahead(ctx, 2)) && _at_token(ctx, _look_ahead(ctx, 3), "R")) {
            /* indirect object <int> <int> R */
            uint64_t obj_num = _read_int(ctx, _shift(ctx));
            uint32_t gen_num = _read_int(ctx, _shift(ctx));
            node = (CosNode*)cos_ref_new(NULL, obj_num, gen_num);
        }
        else {
            /* continue with simple integer */
            PDF_TYPE_INT val = _read_int(ctx, tk1);
            node = (CosNode*)cos_int_new(NULL, val);
        }
        break;
    case COS_TK_NAME:
        node = (CosNode*) _read_name(ctx, tk1);
        break;
    case COS_TK_REAL:
        PDF_TYPE_REAL val = _read_real(ctx, tk1);
        node = (CosNode*)cos_real_new(NULL, val);
        break;
    case COS_TK_WORD:
        switch (tk1->len) {
        case 4:
            if (_at_token(ctx, tk1, "true")) {
                node = (CosNode*)cos_bool_new(NULL, 1);
            }
            else if (_at_token(ctx, tk1, "null")) {
                node = (CosNode*)cos_null_new(NULL);
            }
            break;
        case 5:
            if (_at_token(ctx, tk1, "false")) {
                node = (CosNode*)cos_bool_new(NULL, 0);
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
        }
        case ']': case '>':
        case '{': case '}':
            break;
        default:
            fprintf(stderr, "Unhandled delimiter: %d\n", ctx->buf[tk1->pos]);
            break;
        }
        break;
    case COS_TK_DONE:
        fprintf(stderr, "ended at position %ld\n", ctx->buf_pos);
        break;
    default:
        fprintf(stderr, "todo parse token type %d at position %ld\n", tk1->type, tk1->pos);
        break;
    }

    if (ctx->n_tk) _shift(ctx);

    return node;
}

static CosNode** _parse_objects(CosParserCtx* ctx, size_t* n, char *stopper) {
    CosNode* object;
    CosNode** objects;
    size_t i = *n;
    CosTk* tk = _look_ahead(ctx,1);

    if (_at_token(ctx, tk, stopper)) {
        if (n) {
            objects = malloc(*n * sizeof(CosNode*));
            memset(objects, 0, *n * sizeof(CosNode*));
        }
    }
    else {
        (*n)++;
        object = _parse_object(ctx);
        if (object) {
            objects = _parse_objects(ctx, n, stopper);
        }
        else {
            objects = malloc(*n * sizeof(CosNode*));
            memset(objects, 0, *n * sizeof(CosNode*));
        }
        objects[i] = object;
    }

    return objects;
}

DLLEXPORT CosIndObj* cos_parse_ind_obj(CosNode* self, char* in_buf, size_t in_len) {
    CosTk tk1 = {COS_TK_START, 0, 0}, tk2 = {COS_TK_START, 0, 0}, tk3 = {COS_TK_START, 0, 0};
    CosParserCtx _ctx = { in_buf, in_len, 0, {&tk1, &tk2, &tk3}, 0};
    CosParserCtx* ctx = &_ctx;
    CosIndObj* ind_obj = NULL;

    _look_ahead(ctx, 3); /* load tokens: tk1, tk2, tk3 */

    if (_at_uint(ctx, &tk1) && _at_uint(ctx, &tk2) && _at_token(ctx, &tk3, "obj") ) {
        /* valid indirect object header: <int> <int> obj */
        uint64_t obj_num = _read_int(ctx, &tk1);
        uint32_t gen_num = _read_int(ctx, &tk2);
        CosNode* object;

        _advance(ctx); /* done parsing header */
        object = _parse_object(ctx);

        if (object && object->type == COS_NODE_DICT) {
            CosDict* dict = (void*)object;
            int dos_mode;
            if ( _get_token(ctx, "stream") && _scan_new_line(ctx, &dos_mode)) {
                CosStream* stream = cos_stream_new(NULL, dict, NULL, 0);
                stream->src_buf.pos    = ctx->buf_pos;
                stream->src_buf.is_dos = dos_mode;
                object = (void*) stream;
            }
        }
        if (object) {
            if (object->type == COS_NODE_STREAM || _get_token(ctx, "endobj")) {
                ind_obj = cos_ind_obj_new(NULL, obj_num, gen_num, object);
            }
            else {
                cos_node_done(object);
            }
        }
    }

    return ind_obj;
}

DLLEXPORT CosNode* cos_parse_obj(CosNode* self, char *in_buf, size_t in_len) {
    CosTk tk1 = {COS_TK_START, 0, 0}, tk2 = {COS_TK_START, 0, 0}, tk3 = {COS_TK_START, 0, 0};
    CosParserCtx ctx = { in_buf, in_len, 0, {&tk1, &tk2, &tk3}, 0};
    return _parse_object(&ctx);
}
