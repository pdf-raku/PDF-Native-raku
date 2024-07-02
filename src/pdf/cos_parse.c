#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/cos_parse.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

typedef enum {
    COS_TK_NONE,
    COS_TK_DONE,
    COS_TK_FAIL,
    COS_TK_INT, /* 3 */
    COS_TK_REAL,
    COS_TK_NAME,
    COS_TK_OP,
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
    CosTk tk[3]; /* small look-ahead buffer */
    uint8_t n_tk;
} CosParserCtx;

static CosTk TkNone = {COS_TK_NONE, 0, 0};

static void _tk_cpy(CosTk *dest, CosTk *src) {
        dest->type  = src->type;
        dest->pos = src->pos;
        dest->len   = src->len;
}

static void _ctx_shift(CosParserCtx* ctx) {
    if (ctx->n_tk) {
        if (ctx->n_tk >= 2) {
            _tk_cpy( &ctx->tk[0], &ctx->tk[1]);
            if (ctx->n_tk >= 3) _tk_cpy( &ctx->tk[1], &ctx->tk[2]);
        }
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
        else if (!isspace(ch)) {
            return;
        }
    }
}

/* Return next object or bare word */
static void _ctx_scan_tk(CosParserCtx* ctx) {
    CosTk* tk = &ctx->tk[ctx->n_tk++];
    char prev_ch = ' ';
    int wb = 0;

    _ctx_skip_ws(ctx);

    tk->type = COS_TK_NONE;
    tk->pos  = ctx->buf_pos;
    tk->len  = 0;

    for (; ctx->buf_pos <= ctx->buf_len && tk->type != COS_TK_FAIL && !wb; ctx->buf_pos++) {
        char ch = ctx->buf[ctx->buf_pos];
        tk->len++;
        switch (ch) {
        case '+': case '-':
            if (tk->type == COS_TK_NONE) {
                tk->type = COS_TK_INT;
            }
            else {
                tk->type = COS_TK_FAIL;
            }
            break;
        case '.':
            if (tk->type == COS_TK_NONE || tk->type == COS_TK_INT) {
                tk->type = COS_TK_REAL;
            }
            else {
                tk->type = COS_TK_FAIL;
            }
            break;
        case '0'...'9':
            if (tk->type == COS_TK_NONE) {
                tk->type = COS_TK_INT;
            }
            else if (tk->type != COS_TK_INT && tk->type != COS_TK_REAL && tk->type != COS_TK_WORD) {
                tk->type = COS_TK_FAIL;
            }   
            break;
        case '/':
            if (tk->type == COS_TK_NONE) {
                tk->type = COS_TK_NAME;
            }
            else {
                tk->type = COS_TK_FAIL;
            }
            break;
        case '#':
            if (tk->type !=  COS_TK_NAME) {
                tk->type = COS_TK_FAIL;
            }
            break;
        case '<': case '>': case '[': case ']': case '(': case ')':
            if (tk->type == COS_TK_NONE) {
                tk->type = COS_TK_OP;
            }
            else if (tk->type == COS_TK_OP) {
                /* '<<' and '>>' are the only coalescing operators */
                if (!(ch == '>' || ch == '<') && prev_ch == ch) wb = 1;
            }
            break;
        case 'a'...'z': case 'A'...'Z': case '*': case '"': case '\'':
            if (tk->type == COS_TK_OP) {
                wb = 1;
            }
            else if (tk->type != COS_TK_NONE && tk->type != COS_TK_WORD) {
                tk->type = COS_TK_FAIL;
            }
            else {
                tk->type = COS_TK_WORD;
            }
            break;
        default:
            if (isspace(ch) || ch == '%' || tk->type == COS_TK_OP ) {
                /* if we find a word, leave it on stack for analysis */
                wb = 1;
            }
            else if (tk->type != COS_TK_NAME) {
                /* todo: too permissive */
                tk->type = COS_TK_FAIL;
            }
            break;
        }
        prev_ch = ch;
    }

    ctx->buf_pos--;
    tk->len--;

    if (tk->type == COS_TK_NONE) tk->type = COS_TK_DONE;
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

static int _ctx_looking_ahead(CosParserCtx* ctx) {
    if (ctx->tk[0].type == COS_TK_INT
        && (ctx->n_tk == 1 || (ctx->n_tk == 2 && (ctx->tk[1].type == COS_TK_INT)))) {
        /* e.g. could be <int> <int> R for an indirect reference */
        return 1;
    }
    return 0;
}

static int _at_word(CosParserCtx* ctx, CosTk* tk, char* word) {
       return tk->len == strlen(word) && strncmp(ctx->buf + tk->pos, word, tk->len) == 0;
}

static int _find_word(CosParserCtx* ctx, char* word) {
    int found = 0;
    if (ctx->n_tk == 0) _ctx_scan_tk(ctx);
    if (ctx->n_tk > 0) {
        CosTk* tk = & ctx->tk[0];
        found = _at_word(ctx, tk, word);
        if (found) _ctx_shift(ctx);
    }
    return found;
}

static void _ctx_scan(CosParserCtx* ctx) {
    while (ctx->buf_pos < ctx->buf_len && (ctx->n_tk == 0 || _ctx_looking_ahead(ctx))) {
        _ctx_scan_tk(ctx);
    }
}

static void _ctx_flush(CosParserCtx* ctx) {
    if (ctx->n_tk) {
        CosTk* tk = &ctx->tk[ ctx->n_tk - 1];
        ctx->buf_pos = tk->pos + tk->len + 1;
        ctx->n_tk = 0;
    }
}

static CosNode* _parse_obj(CosParserCtx* ctx) {
    CosTk* tk1 = &ctx->tk[0];
    CosTk* tk2 = &ctx->tk[1];
    CosTk* tk3 = &ctx->tk[1];
    CosNode* node = NULL;

    _ctx_scan(ctx);

    switch (tk1->type) {
    case COS_TK_INT:
        if (ctx->n_tk == 3 && tk2->type == COS_TK_INT && _at_word(ctx, tk3, "R")) {
            /* indirect object <int> <int> R */
            uint64_t obj_num = _ctx_read_int(ctx, tk1);
            uint32_t gen_num = _ctx_read_int(ctx, tk2);
            node = (CosNode*)cos_ref_new(NULL, obj_num, gen_num);
            ctx->n_tk = 0;
        }
        else {
            /* continue with simple integer */
            PDF_TYPE_INT val = _ctx_read_int(ctx, tk1);
            node = (CosNode*)cos_int_new(NULL, val);
            _ctx_shift(ctx);
        }
        break;
    case COS_TK_FAIL:
        fprintf(stderr, "got parse error at position %ld\n", ctx->buf_pos);
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

DLLEXPORT CosIndObj* cos_parse_ind_obj(CosNode* self, char *in_buf, size_t in_len) {
    CosParserCtx _ctx = { in_buf, in_len, 0, {{COS_TK_NONE, 0, 0}, {COS_TK_NONE, 0, 0}, {COS_TK_NONE, 0, 0}}, 0};
    CosParserCtx* ctx = &_ctx;
    CosTk* tk1 = &ctx->tk[0];
    CosTk* tk2 = &ctx->tk[1];
    CosTk* tk3 = &ctx->tk[2];
    CosIndObj* ind_obj = NULL;

    _ctx_scan(ctx);

    if (ctx->n_tk == 3 && tk1->type == COS_TK_INT && tk2->type == COS_TK_INT && _at_word(ctx, tk3, "obj") ) {
        /* at indirect object starter: <int> <int> obj */
        uint64_t obj_num = _ctx_read_int(ctx, tk1);
        uint32_t gen_num = _ctx_read_int(ctx, tk2);
        CosNode* object;
        ctx->n_tk = 0;
        object = _parse_obj(ctx);

        if (object) {
            /* todo detect <dict> followed by stream ... endstream */
            if ( _find_word(ctx, "endobj")) {
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
    CosParserCtx ctx = { in_buf, in_len, 0, {{COS_TK_NONE, 0, 0}, {COS_TK_NONE, 0, 0}, {COS_TK_NONE, 0, 0}}, 0};
    return _parse_obj(&ctx);
}
