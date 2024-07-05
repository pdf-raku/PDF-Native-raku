#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/cos_parse.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

typedef enum {
    COS_TK_START,
    COS_TK_DONE,
    COS_TK_DELIM,
    COS_TK_INT, /* 3 */
    COS_TK_REAL,
    COS_TK_NAME,
    COS_TK_WORD, /* 6 */
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
    char prev_ch = ' ';
    int wb = 0;

    assert(ctx->n_tk < 3);

    tk = ctx->tk[ctx->n_tk++];
    _skip_ws(ctx);

    tk->type = COS_TK_START;
    tk->pos  = ctx->buf_pos;
    tk->len  = 0;

    for (; ctx->buf_pos < ctx->buf_len && !wb; ctx->buf_pos++) {
        char ch = ctx->buf[ctx->buf_pos];
        tk->len++;
        switch (ch) {
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
        case '0'...'9':
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
        case '[': case ']':
        case '(': case ')':
        case '{': case '}':
            if (tk->type == COS_TK_START) {
                tk->type = COS_TK_DELIM;
            }
            else if (tk->type == COS_TK_DELIM) {
                /* allow just '<<' and '>>' as 2 character delimiters */
                if (!(ch == '>' || ch == '<') && prev_ch == ch) wb = 1;
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

    if (ctx->n_tk == 0)  _look_ahead(ctx, 1);
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
    size_t i;

    assert(tk->type == COS_TK_INT);

    for (i = 0; i < tk->len; i++) {
        val *= 10;
        val += ctx->buf[tk->pos + i] - '0';
    }
    return val;
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

    return val  +  frac / magn;
}

static int _at_token(CosParserCtx* ctx, CosTk* tk, char* word) {
    return tk->len == strlen(word) && strncmp(ctx->buf + tk->pos, word, tk->len) == 0;
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

static int _get_hex_value(char** pos, char* end) {

    do {
        (*pos)++;
    }
    while (isspace(**pos) && *pos < end);

    if (*pos < end) {
        switch (**pos) {
        case '0'...'9':
            return **pos - '0';
        case 'A'...'F':
            return **pos - 'A' + 10;
        case 'a'...'f':
            return **pos - 'a' + 10;
        default:
            return -1;
        }
    }
    return 0;
}

static CosHexString* _parse_hex_string(CosParserCtx* ctx) {
    CosHexString* hex_string = NULL;
    char* hex_pos = ctx->buf + ctx->buf_pos - 1;
    char* hex_end = _strnchr(hex_pos, '>', ctx->buf_len - ctx->buf_pos + 1);
    size_t n = 0;
    if (hex_end) {
        /* create our object early to get a buffer */
        size_t max_bytes = (hex_end - hex_pos + 1) / 2; /* two hex chars per byte   */
        PDF_TYPE_STRING hex_bytes = malloc(max_bytes);
        while (hex_pos < hex_end) {
            int d1, d2;
            d1 = _get_hex_value(&hex_pos, hex_end);
            if (d1 < 0) goto bail;
            if (hex_pos >= hex_end) break;
            d2 = _get_hex_value(&hex_pos, hex_end);
            if (d2 < 0 || n >= max_bytes) goto bail;
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

static CosNode* _parse_object(CosParserCtx* ctx) {
    CosNode* node = NULL;
    CosTk* tk1 = _look_ahead(ctx, 1);
    char ch = ctx->buf[tk1->pos];

    switch (tk1->type) {
    case COS_TK_INT:
        if (_look_ahead(ctx, 2)->type == COS_TK_INT && _at_token(ctx, _look_ahead(ctx, 3), "R")) {
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
            {   size_t n = 0;
                CosNode** objects = _parse_objects(ctx, &n, "]");
                if (_get_token(ctx, "]") && (n == 0 || objects[n-1] != NULL)) {
                    node = (CosNode*) cos_array_new(NULL, objects, n);
                }
                _done_objects(objects, n);
            }
            break;
        }
        case '<': {
            switch (tk1->len) {
            case 1: /* '<' hex string '>' */
                node = (void*) _parse_hex_string(ctx);
                break;
            case 2: /* '<<' dictionary '>>' */
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
            objects = malloc(*n * sizeof(CosTk*));
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

    if (tk1.type == COS_TK_INT && tk2.type == COS_TK_INT && _at_token(ctx, &tk3, "obj") ) {
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
