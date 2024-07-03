#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/cos_parse.h"
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

typedef enum {
    COS_TK_START,
    COS_TK_DONE,
    COS_TK_DELIM,
    COS_TK_INT, /* 3 */
    COS_TK_REAL,
    COS_TK_NAME,
    COS_TK_WORD, /* 7 */
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

static void _ctx_advance(CosParserCtx* ctx) {
    assert(ctx->n_tk >= 1);
    if (ctx->n_tk) {
        /* recycle buffers */
        CosTk* tmp = ctx->tk[0];
        ctx->tk[0] = ctx->tk[1];
        ctx->tk[1] = ctx->tk[2];
        ctx->tk[2] = tmp;
        ctx->n_tk--;
    }
}

static void _ctx_skip_ws(CosParserCtx* ctx) {
    int in_comment = 0;
    for (; ctx->buf_pos <= ctx->buf_len; ctx->buf_pos++) {
        char ch = ctx->buf[ctx->buf_pos];
        if (in_comment) {
            if (ch == '\n' || ch == '\r') in_comment = 0;
        }
        else if (ch == '%') {
            in_comment = 1;
        }
        else if (ch && !isspace(ch)) {
            return;
        }
    }
}

/* Scan for the next start token: */
/* - preceding comments and whitespace are skipped */
/* - names, numbers, words and indirect references are returned as tokens */
/* - returns opening delimiter for strings, arrays and dictionaries */
static void _ctx_scan_tk(CosParserCtx* ctx) {
    CosTk* tk;
    char prev_ch = ' ';
    int wb = 0;

    assert(ctx->n_tk < 3);

    tk = ctx->tk[ctx->n_tk++];
    _ctx_skip_ws(ctx);

    tk->type = COS_TK_START;
    tk->pos  = ctx->buf_pos;
    tk->len  = 0;

    for (; ctx->buf_pos <= ctx->buf_len && !wb; ctx->buf_pos++) {
        char ch = ctx->buf[ctx->buf_pos];
        tk->len++;
        switch (ch) {
        case '+': case '-':
            if (tk->type == COS_TK_START) {
                tk->type = COS_TK_INT;
            }
            else if (tk->type != COS_TK_WORD && tk->type != COS_TK_NAME) {
                wb = 1;
            }
            break;
        case '.':
            if (tk->type == COS_TK_START || tk->type == COS_TK_INT) {
                tk->type = COS_TK_REAL;
            }
            else if (tk->type != COS_TK_WORD && tk->type != COS_TK_NAME)  {
                wb = 1;
            }
            break;
        case '0'...'9':
            if (tk->type == COS_TK_START) {
                tk->type = COS_TK_INT;
            }
            else if (tk->type == COS_TK_DELIM) {
                wb = 1;
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
        case '#':
            if (tk->type != COS_TK_NAME) {
                if (tk->type == COS_TK_START) {
                    tk->type = COS_TK_WORD;
                }
                else if (tk->type != COS_TK_WORD) {
                    wb = 1;
                }
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
        case 'a'...'z': case 'A'...'Z': case '*': case '"': case '\'':
            if (tk->type == COS_TK_START || tk->type == COS_TK_INT || tk->type == COS_TK_REAL) {
                tk->type = COS_TK_WORD;
            }
            else if (tk->type != COS_TK_NAME && tk->type != COS_TK_WORD) {
                wb = 1;
            }
            break;
        default:
            if (!ch || ch == '%' || isspace(ch) || tk->type == COS_TK_DELIM) {
                wb = 1;
            }
            else if (tk->type != COS_TK_NAME) {
                tk->type = COS_TK_WORD;
            }
            break;
        }
        prev_ch = ch;
    }

    ctx->buf_pos--;
    tk->len--;

    if (tk->type == COS_TK_START) tk->type = COS_TK_DONE;
}

static CosTk* _ctx_look_ahead(CosParserCtx* ctx, int n) {
    assert(n >= 1 && n <= 3);

    while (ctx->n_tk < n) {
        _ctx_scan_tk(ctx);
    }
    return ctx->tk[n - 1];
}

static PDF_TYPE_INT _ctx_read_int(CosParserCtx* ctx, CosTk* tk) {
    PDF_TYPE_INT val = 0;
    size_t i;
    for (i = 0; i < tk->len; i++) {
        val *= 10;
        val += ctx->buf[tk->pos + i] - '0';
    }
    return val;
}

static int _at_word(CosParserCtx* ctx, CosTk* tk, char* word) {
    return tk->len == strlen(word) && strncmp(ctx->buf + tk->pos, word, tk->len) == 0;
}

static int _scan_word(CosParserCtx* ctx, char* word) {
    int found = 0;
    CosTk* tk = _ctx_look_ahead(ctx, 1);
    found = _at_word(ctx, tk, word);
    if (found) _ctx_advance(ctx);

    return found;
}

static CosNode* _parse_obj(CosParserCtx* ctx) {
    CosNode* node = NULL;
    CosTk* tk1 = _ctx_look_ahead(ctx, 1);

    switch (tk1->type) {
    case COS_TK_INT:
        CosTk* tk2 = _ctx_look_ahead(ctx, 2);
        if (tk2->type == COS_TK_INT && _at_word(ctx, _ctx_look_ahead(ctx, 3), "R")) {
            /* indirect object <int> <int> R */
            uint64_t obj_num = _ctx_read_int(ctx, tk1);
            uint32_t gen_num = _ctx_read_int(ctx, tk2);
            node = (CosNode*)cos_ref_new(NULL, obj_num, gen_num);
            ctx->n_tk = 0;
        }
        else {
            /* continue with simple integer */
            PDF_TYPE_INT val = _ctx_read_int(ctx, tk1);
            _ctx_advance(ctx);
            node = (CosNode*)cos_int_new(NULL, val);
        }
        break;
    case COS_TK_DONE:
        fprintf(stderr, "ended at position %ld\n", ctx->buf_pos);
        break;
    default:
        fprintf(stderr, "todo parse token type %d\n", tk1->type);
        break;
    }

    return node;
}

DLLEXPORT CosIndObj* cos_parse_ind_obj(CosNode* self, char* in_buf, size_t in_len) {
    CosTk tk1 = {COS_TK_START, 0, 0}, tk2 = {COS_TK_START, 0, 0}, tk3 = {COS_TK_START, 0, 0};
    CosParserCtx _ctx = { in_buf, in_len, 0, {&tk1, &tk2, &tk3}, 0};
    CosParserCtx* ctx = &_ctx;
    CosIndObj* ind_obj = NULL;

    _ctx_look_ahead(ctx, 3);

    if (tk1.type == COS_TK_INT && tk2.type == COS_TK_INT && _at_word(ctx, &tk3, "obj") ) {
        /* at indirect object starter: <int> <int> obj */
        uint64_t obj_num = _ctx_read_int(ctx, &tk1);
        uint32_t gen_num = _ctx_read_int(ctx, &tk2);
        CosNode* object;
        ctx->n_tk = 0;
        object = _parse_obj(ctx);

        if (object) {
            /* todo detect <dict> followed by stream ... endstream */
            if ( _scan_word(ctx, "endobj")) {
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
    return _parse_obj(&ctx);
}
