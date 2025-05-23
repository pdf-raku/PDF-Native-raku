#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/write.h"
#include "pdf/_bufcat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#define COS_CHECK(n) ((n)->type + 123)

DLLEXPORT void cos_node_reference(CosNode* self) {
    if (self == NULL) return;
    if (self->check != COS_CHECK(self)) {
        fprintf(stderr, __FILE__ ":%d node %p (type %d) not owned by us or corrupted\n", __LINE__, (void*) self, self->type);
        return;
    }
    self->ref_count++;
}

DLLEXPORT void cos_node_done(CosNode* self) {
    if (self == NULL) return;
    if (self->check != COS_CHECK(self)) {
        fprintf(stderr, __FILE__ ":%d node %p (type %d, ref %d) not owned by us or corrupted\n", __LINE__, (void*) self, self->type, self->ref_count);
    }
    else if (self->ref_count == 0) {
        fprintf(stderr, __FILE__ ":%d node was not referenced: %p\n", __LINE__, (void*) self);
    }
    else if (--(self->ref_count) <= 0) {
        switch ((CosNodeType)self->type) {
        case COS_NODE_ANY:
        case COS_NODE_BOOL:
        case COS_NODE_INT:
        case COS_NODE_NULL:
        case COS_NODE_REAL:
        case COS_NODE_REF:
            /* leaf node */
            break;
        case COS_NODE_NAME:
            free(((CosName*)self)->value);
            break;
        case COS_NODE_LIT_STR:
        case COS_NODE_HEX_STR:
        case COS_NODE_COMMENT:
            free(((struct CosStringyNode*)self)->value);
            break;
        case COS_NODE_OP:
            free(((CosOp*)self)->opn);
            /* fallthrough */
        case COS_NODE_CONTENT:
        case COS_NODE_ARRAY:
            {
                size_t i;
                struct CosContainerNode* a = (void*)self;
                for (i=0; i < a->elems; i++) {
                    cos_node_done(a->values[i]);
                }
                free(a->values);
            }
            break;
        case COS_NODE_DICT:
             {
                size_t i;
                CosDict* a = (void*)self;
                for (i=0; i < a->elems; i++) {
                    cos_node_done((CosNode*)a->keys[i]);
                    cos_node_done(a->values[i]);
                }
                free(a->keys);
                free(a->values);
                if (a->index) free(a->index);
            }
            break;
        case COS_NODE_IND_OBJ:
            cos_node_done(((CosIndObj*)self)->value);
            break;
        case COS_NODE_STREAM:
        case COS_NODE_INLINE_IMAGE:
            {
                struct CosStreamish* stream = (void*)self;
                cos_node_done((CosNode*)stream->dict);
                if (stream->value) free(stream->value);
            }
            break;
        }
        free((void*)self);
    }
}

#define COS_CMP(v1,v2) ((v1) == (v2) ? COS_CMP_EQUAL : COS_CMP_DIFFERENT)

static int _cmp_code_points(PDF_TYPE_CODE_POINTS v1, PDF_TYPE_CODE_POINTS v2, size_t key_len) {
    uint16_t i;
    for (i = 0; i < key_len; i++) {
        if (v1[i] != v2[i]) {
            return v1[i] > v2[i] ? 1 : -1;
        }
    }
    return 0;
}

static int _cmp_chars(char* v1, char* v2, uint16_t key_len) {
    uint16_t i;
    for (i = 0; i < key_len; i++) {
        if (v1[i] != v2[i]) {
            return ((unsigned char)v1[i] > (unsigned char)v2[i]
                    ? 1 : -1);
        }
    }
    return COS_CMP_EQUAL;
}


static int _int_value(CosNode *node, PDF_TYPE_INT *value) {
    switch (node->type) {
    case COS_NODE_INT:
        *value = ((CosInt*)node)->value;
        return 1;
    case COS_NODE_REAL:
    {
        PDF_TYPE_REAL rval = ((CosReal*)node)->value;
        *value = rval;
        return (rval - *value) == 0;
    }
    }
    return 0;
}

static int _is_stringy_node(CosNode* self) {
    return self != NULL && (self->type == COS_NODE_LIT_STR || self->type == COS_NODE_HEX_STR || self->type == COS_NODE_COMMENT);
}

static int _is_numeric_node(CosNode* self) {
    return self != NULL && (self->type == COS_NODE_INT || self->type == COS_NODE_REAL);
}

/* simple strict comparison types and object order must match */
DLLEXPORT int cos_node_cmp(CosNode* self, CosNode* obj) {
    if (self == obj) {
        return COS_CMP_EQUAL;
    }
    else if (self == NULL || obj == NULL) {
        return COS_CMP_DIFFERENT_TYPE;
    }
    else if (self->type != obj->type) {
        PDF_TYPE_INT v1, v2;
        if (_is_numeric_node(self) && _is_numeric_node(obj)) {
            /* presumably real vs integer */
            return (_int_value(self, &v1) && _int_value(obj, &v2) && v1 == v2)
                ? COS_CMP_SIMILAR
                : COS_CMP_DIFFERENT;
        }
        else if (_is_stringy_node(self) && _is_stringy_node(obj)) {
            struct CosStringyNode* a = (void*)self;
            struct CosStringyNode* b = (void*)obj;
            if (a->value_len == b->value_len
                && _cmp_chars(a->value, b->value, a->value_len) == COS_CMP_EQUAL) {
                return COS_CMP_SIMILAR;
            }
            else {
                return COS_CMP_DIFFERENT;
            }
        }
        return COS_CMP_DIFFERENT_TYPE;
    }
    else {
        /* both nodes are non-null and the same type */
        switch ((CosNodeType)self->type) {
            case COS_NODE_BOOL:
                return COS_CMP(((CosBool*)self)->value, ((CosBool*)obj)->value);
            case COS_NODE_INT:
                return COS_CMP(((CosInt*)self)->value, ((CosInt*)obj)->value);
            case COS_NODE_ANY:
            case COS_NODE_NULL:
                return COS_CMP_EQUAL;
            case COS_NODE_REAL:
                return COS_CMP(((CosReal*)self)->value, ((CosInt*)obj)->value);
            case COS_NODE_REF:
            {
                CosRef* a = (void*)self;
                CosRef* b = (void*)obj;
                return (a->obj_num == b->obj_num && a->gen_num == b->gen_num)
                    ? COS_CMP_EQUAL
                    : COS_CMP_DIFFERENT;
            }
            case COS_NODE_NAME:
            {
                CosName* a = (void*)self;
                CosName* b = (void*)obj;
                return ((a->value_len == b->value_len) && !_cmp_code_points(a->value, b->value, a->value_len))
                    ? COS_CMP_EQUAL
                    : COS_CMP_DIFFERENT;
            }
            case COS_NODE_LIT_STR:
            case COS_NODE_HEX_STR:
            case COS_NODE_COMMENT:
            {
                struct CosStringyNode* a = (void*)self;
                struct CosStringyNode* b = (void*)obj;
                return ((a->value_len == b->value_len) && !_cmp_chars(a->value, b->value, a->value_len))
                    ? COS_CMP_EQUAL
                    : COS_CMP_DIFFERENT;
            }
            case COS_NODE_OP:
                if (strcmp(((CosOp*)self)->opn, ((CosOp*)obj)->opn)) return  COS_CMP_DIFFERENT;
                /* fallthrough */
            case COS_NODE_CONTENT:
            case COS_NODE_ARRAY:
            {
                struct CosContainerNode* a = (void*)self;
                struct CosContainerNode* b = (void*)obj;
                int rv = COS_CMP_EQUAL;
                size_t i;
                if (a->elems != b->elems) return COS_CMP_DIFFERENT;
                for (i = 0; i < a->elems; i++) {
                    int cmp = cos_node_cmp(a->values[i], b->values[i]);
                    if (cmp == COS_CMP_SIMILAR) {
                        rv = cmp;
                    }
                    else if (cmp >= COS_CMP_DIFFERENT) {
                        return COS_CMP_DIFFERENT;
                    }
                }
                return rv;
            }
            case COS_NODE_DICT:
            {
                CosDict* a = (void*)self;
                CosDict* b = (void*)obj;
                int rv = COS_CMP_EQUAL;
                size_t i;
                if (!a->index) cos_dict_build_index(a);
                if (!b->index) cos_dict_build_index(b);
                if (a->index_len != b->index_len) return COS_CMP_DIFFERENT;
                for (i = 0; i < a->index_len; i++) {
                    size_t ai = a->index[ i ];
                    size_t bi = b->index[ i ];
                    if (ai != bi) rv = COS_CMP_SIMILAR; /* keys in different order */
                    if (cos_node_cmp((CosNode*)a->keys[ai], (CosNode*)b->keys[bi])) return COS_CMP_DIFFERENT;
                    {
                        int cmp = cos_node_cmp(a->values[ai], b->values[bi]);
                        if (cmp == COS_CMP_SIMILAR) {
                            rv = cmp;
                        }
                        else if (cmp >= COS_CMP_DIFFERENT) {
                            return COS_CMP_DIFFERENT;
                        }
                    }
                }
                return rv;
            }
            case COS_NODE_IND_OBJ:
            {
                CosIndObj* a = (void*)self;
                CosIndObj* b = (void*)obj;
                int rv = COS_CMP_EQUAL;
                if (a->obj_num != b->obj_num || a->gen_num != b->gen_num) {
                    rv = COS_CMP_DIFFERENT;
                }
                else {
                    rv = cos_node_cmp(a->value, b->value);
                    if (rv > COS_CMP_DIFFERENT) rv = COS_CMP_DIFFERENT;
                }
                return rv;
            }
            case COS_NODE_STREAM:
            case COS_NODE_INLINE_IMAGE:
            {
                struct CosStreamish* a = (void*)self;
                struct CosStreamish* b = (void*)obj;
                int rv = COS_CMP_EQUAL;
                if (!a->value || !b->value) {
                    /* streams not fully loaded */
                    rv = COS_CMP_INVALID;
                }
                else if (a->value_len != b->value_len || _cmp_chars(a->value, b->value, a->value_len)) {
                    rv = COS_CMP_DIFFERENT;
                }
                else {
                    rv = cos_node_cmp((CosNode*)a->dict, (CosNode*)b->dict);
                    if (rv > COS_CMP_DIFFERENT) rv = COS_CMP_DIFFERENT;
                }
                return rv;
            }
        }
        return 0;
    }
}

static int _node_write(CosNode* self, char* out, int out_len, int indent) {
    switch (self ? self->type : COS_NODE_NULL) {
    case COS_NODE_IND_OBJ:
        return cos_ind_obj_write((CosIndObj*)self, out, out_len);
    case COS_NODE_INT:
        return cos_int_write((CosInt*)self, out, out_len);
    case COS_NODE_BOOL:
        return cos_bool_write((CosBool*)self, out, out_len);
    case COS_NODE_NULL:
        return cos_null_write((CosNull*)self, out, out_len);
    case COS_NODE_REAL:
        return cos_real_write((CosReal*)self, out, out_len);
    case COS_NODE_REF:
        return cos_ref_write((CosRef*)self, out, out_len);
    case COS_NODE_ARRAY:
        return cos_array_write((CosArray*)self, out, out_len, indent);
    case COS_NODE_DICT:
        return cos_dict_write((CosDict*)self, out, out_len, indent);
    case COS_NODE_NAME:
        return cos_name_write((CosName*)self, out, out_len);
    case COS_NODE_LIT_STR:
        return cos_literal_write((CosLiteralStr*)self, out, out_len);
    case COS_NODE_HEX_STR:
        return cos_hex_string_write((CosHexString*)self, out, out_len);
    case COS_NODE_STREAM:
        return cos_stream_write((CosStream*)self, out, out_len);
    case COS_NODE_OP:
        return cos_op_write((CosOp*)self, out, out_len, indent);
    case COS_NODE_INLINE_IMAGE:
        return cos_inline_image_write((CosInlineImage*)self, out, out_len, indent);
    case COS_NODE_CONTENT:
        return cos_content_write((CosContent*)self, out, out_len);
    case COS_NODE_COMMENT:
        return cos_comment_write((CosHexString*)self, out, out_len, indent);
    default :
        fprintf(stderr, __FILE__ ":%d type not yet handled: %d\n", __LINE__, self->type);
        return 0;
    }

}


DLLEXPORT CosArray* cos_array_new(CosNode** values, size_t elems) {
    size_t i;
    CosArray* self = malloc(sizeof(CosArray));
    self->type = COS_NODE_ARRAY;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->elems = elems;
    self->values = malloc(sizeof(CosNode*) * elems);
    for (i=0; i < elems; i++) {
        self->values[i] = values[i];
        cos_node_reference(values[i]);
    }
    return self;
}

DLLEXPORT size_t cos_array_write(CosArray* self, char* out, size_t out_len, int indent) {
    size_t n = 0;
    size_t i;
    size_t m;

    n += (m = _bufcat(out, out_len, "[ "));
    if (m == 0) return 0;

    for (i=0; i < self->elems; i++) {
        n += (m = _node_write(self->values[i], out+n, out_len - n, indent));
        if (m == 0 || n >= out_len) return 0;
        out[n++] = ' ';
    }
    if (n >= out_len) return 0;
    out[n++] = ']';

    return n;
}

DLLEXPORT CosDict* cos_dict_new(CosName** keys, CosNode** values, size_t elems) {
    size_t i;
    CosDict* self = malloc(sizeof(CosDict));
    self->type = COS_NODE_DICT;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->elems = elems;
    self->keys   = malloc(sizeof(CosName*) * elems);
    self->values = malloc(sizeof(CosNode*) * elems);
    for (i=0; i < elems; i++) {
        self->keys[i] = keys[i];
        cos_node_reference((CosNode*)keys[i]);

        self->values[i] = values[i];
        cos_node_reference(values[i]);
    }
    self->index = NULL;
    self->index_len = 0;

    return self;
}

/* sorting comparision */
static int _cmp_names(CosName* n1, CosName* n2) {
    if (!n1 || !n2 || n1->type != COS_NODE_NAME || n1->type != COS_NODE_NAME) {
        return 0;
    }
    else if (n1->value_len != n2->value_len) {
        return n1->value_len > n2->value_len ? -1 : 1;
    }
    return _cmp_code_points(n1->value, n2->value, n1->value_len);
}

DLLEXPORT CosNode* cos_dict_lookup(CosDict* self, CosName* key) {
    size_t low, high;
    size_t* index = self->index;

    if (!self->elems) return NULL;

    if (!index) index = cos_dict_build_index(self);

    for (low = 0, high = self->index_len;;) {
        size_t mid = low + (high - low) / 2;
        CosName* this_key = self->keys[ index[mid] ];
        int cmp = _cmp_names(key, this_key);

        if (cmp == 0) {
            return self->values[ index[mid] ];
        }
        else if (low >= high-1) {
            break;
        }
        else if (cmp < 0) {
            high = mid;
        }
        else {
            low = mid;
        }
    }
    return NULL;
}

DLLEXPORT size_t* cos_dict_build_index(CosDict* self) {
    size_t i;

    if (self->index) return self->index;

    self->index = malloc(self->elems * sizeof(size_t) );
    self->index_len = 0;

    /* pass 1: sequence, ignoring nulls */
    for (i = 0; i < self->elems; i++) {
        if (self->values[i]->type != COS_NODE_NULL) {
            self->index[ self->index_len++ ] = i;
        }
    }
    /* pass 2: stable sort */
    {
        int sorted = 0;
        int pass = 0;
        while (!sorted) {
            sorted = 1;
            for (i = 1; i < self->index_len - pass; i++) {
                int cmp = _cmp_names(self->keys[ self->index[i-1] ], self->keys[ self->index[i] ]);
                if (cmp > 0) {
                    size_t tmp = self->index[i];
                    self->index[i] = self->index[i-1];
                    self->index[i-1] = tmp;
                    sorted = 0;
                }
            }
            pass++;
        }
    }

    return self->index;
}

static size_t _indent_items(CosDict* self, char *out, size_t out_len, size_t* pos, int indent) {
    int64_t i;
    int j;

    if (indent <= 0) return 0;

    for (i = self->elems - 1; i >= 0; i--) {
        size_t shift = i * indent;
        size_t elem_len = pos[i+1] - pos[i];
        char* src = out + pos[i];
        char* dest = src + shift + indent;

        if (dest + elem_len >= out + out_len) {
            return 0;
        }

        memmove(dest, src, elem_len);

        for (j = 0; j < indent; j++) {
            *(src + shift + j) = (j == 0 ? '\n' : ' ');
        }
    }

    return indent * self->elems;
}

/* Same as the Raku PDF::IO::Writer::MultiLineDictWrap constant */
#define MultiLineDictWrap 65

DLLEXPORT size_t cos_dict_write(CosDict* self, char* out, size_t out_len, int indent) {
    size_t n = 0;
    size_t i, m;
    size_t* pos = malloc((self->elems+1) * sizeof(size_t));
    int elem_indent = indent >= 0 ? indent + 2 : -1;

    n += (m = _bufcat(out, out_len, "<< "));
    if (m == 0) goto bail;

    for (i = 0; i < self->elems; i++) {
        pos[i] = n - 1;

        n += (m = _node_write((CosNode*)self->keys[i], out+n, out_len - n, 0));
        if (m == 0 || n >= out_len) goto bail;
        out[n++] = ' ';

        n += (m = _node_write(self->values[i], out+n, out_len - n, elem_indent));
        if (m == 0 || n >= out_len) goto bail;
        out[n++] = ' ';
    }

    if (elem_indent > 0 && n - self->elems - 3 >= MultiLineDictWrap) {
        pos[self->elems] = n;
        n += (m = _indent_items(self, out, out_len, pos, elem_indent));
        if (m == 0 || n + indent >= out_len) goto bail;

        out[n-1] = '\n';
        for (; indent > 0; indent--) {
            if (n >= out_len) goto bail;
            out[n++] = ' ';
        }
    }

    n += (m = _bufcat(out+n, out_len-n, ">>"));
    if (m == 0) goto bail;

    free(pos);
    return n;

bail:
    free(pos);
    return 0;
}

DLLEXPORT CosRef* cos_ref_new(uint64_t obj_num, uint32_t gen_num) {
    CosRef* self = malloc(sizeof(CosRef));
    self->type = COS_NODE_REF;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    return self;
}

DLLEXPORT size_t cos_ref_write(CosRef* self, char* out, size_t out_len) {
    return snprintf(out, out_len, "%" PRId64 " %d R", self->obj_num, self->gen_num);
}

DLLEXPORT CosIndObj* cos_ind_obj_new(uint64_t obj_num, uint32_t gen_num, CosNode* value) {
    CosIndObj* self = malloc(sizeof(CosIndObj));
    self->type = COS_NODE_IND_OBJ;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    self->value = value;
    cos_node_reference(value);
    return self;
}

DLLEXPORT size_t cos_ind_obj_write(CosIndObj* self, char* out, size_t out_len) {
    size_t n = 0;
    size_t m;
    n = snprintf(out, out_len, "%" PRId64 " %d obj\n", self->obj_num, self->gen_num);
    n += (m = _node_write(self->value, out+n, out_len-n, 0));
    if (m == 0) return 0;
    n += (m = _bufcat(out+n, out_len-n, "\nendobj\n"));

    return m ? n : 0;
}

static void _crypt_node(CosNode* self, CosCryptNodeCtx* crypt_ctx) {
    switch (self->type) {
        case COS_NODE_LIT_STR:
        case COS_NODE_HEX_STR:
            if (crypt_ctx->mode != COS_CRYPT_ONLY_STREAMS) {
                struct CosStringyNode* s = (void*) self;
                crypt_ctx->crypt_cb(crypt_ctx, s->value, s->value_len);
            }
            break;
        case COS_NODE_ARRAY:
        case COS_NODE_DICT:
            {
                size_t i;
                struct CosContainerNode* a = (void*)self;
                for (i=0; i < a->elems; i++) {
                    _crypt_node(a->values[i], crypt_ctx);
                }
            }
            break;
        case COS_NODE_STREAM:
            {
                CosStream* s = (void*) self;
                _crypt_node((CosNode*)s->dict, crypt_ctx);

                if (crypt_ctx->mode != COS_CRYPT_ONLY_STRINGS) {
                    crypt_ctx->crypt_cb(crypt_ctx, s->value, s->value_len);
                }
            }
            break;
        default :
            break;
    }
}

DLLEXPORT CosCryptNodeCtx* cos_crypt_ctx_new(CosCryptFunc crypt_cb, CosCryptMode mode, unsigned char* key, int key_len) {
    CosCryptNodeCtx* self = malloc(sizeof(CosCryptNodeCtx));
    self->key = malloc(key_len);
    memcpy(self->key, key, key_len);
    self->key_len = key_len;
    self->mode = mode;
    self->crypt_cb = crypt_cb;
    self->buf_len = 512;
    self->buf = malloc(self->buf_len);
    self->obj_num = 0;
    self->gen_num = 0;

    return self;
}

DLLEXPORT void cos_crypt_ctx_done(CosCryptNodeCtx* self) {
    if (self->key) free(self->key);
    if (self->buf) free(self->buf);
    free(self);
}

DLLEXPORT void cos_ind_obj_crypt(CosIndObj* self, CosCryptNodeCtx* crypt_ctx) {
    crypt_ctx->obj_num = self->obj_num;
    crypt_ctx->gen_num = self->gen_num;
    _crypt_node(self->value, crypt_ctx);
    crypt_ctx->obj_num = 0;
    crypt_ctx->gen_num = 0;
}

DLLEXPORT CosStream* cos_stream_new(CosDict* dict, unsigned char* value, size_t value_len) {
    CosStream* self = malloc(sizeof(CosStream));
    self->type = COS_NODE_STREAM;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->dict = dict;
    cos_node_reference((CosNode*)dict);

    if (value) {
        self->value = malloc(value_len);
        memcpy(self->value, value, value_len);
        self->value_len = value_len;
    }
    else {
        self->value = NULL;
        self->value_pos = value_len;
    }

    return self;
}

/* continue read from parse buffer, attaching stream data */
DLLEXPORT int cos_stream_attach_data(CosStream* self, unsigned char* buf, size_t buf_len, size_t value_len) {
    if (self->value) return -1;
    if (self->value_pos + value_len > buf_len) return -1;
    self->value = malloc(value_len);
    memcpy(self->value, buf + self->value_pos, value_len);
    self->value_len = value_len;
    return 1;
}

DLLEXPORT size_t cos_stream_write(CosStream* self, char* out, size_t out_len) {
    size_t m;
    size_t n = cos_dict_write(self->dict, out, out_len, 0);

    n += (m = _bufcat(out+n, out_len-n, " stream\n"));
    if (m == 0) return 0;

    if (self->value) {
        size_t i;
        for (i = 0; i < self->value_len && n < out_len; i++) {
            out[n++] = self->value[i];
        }

        n += (m = _bufcat(out+n, out_len-n, "\nendstream"));
        if (m == 0) return 0;
    }

    return n;
}

DLLEXPORT CosInt* cos_int_new(PDF_TYPE_INT value) {
    CosInt* self = malloc(sizeof(CosInt));
    self->type = COS_NODE_INT;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_int_write(CosInt* self, char* out, size_t out_len) {
    return  pdf_write_int(self->value, out, out_len);
}

DLLEXPORT CosBool* cos_bool_new(PDF_TYPE_BOOL value) {
    CosBool* self = malloc(sizeof(CosBool));
    self->type = COS_NODE_BOOL;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_bool_write(CosBool* self, char* out, size_t out_len) {
    return  pdf_write_bool(self->value, out, out_len);
}

DLLEXPORT CosReal* cos_real_new(PDF_TYPE_REAL value) {
    CosReal* self = malloc(sizeof(CosReal));
    self->type = COS_NODE_REAL;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_real_write(CosReal* self, char* out, size_t out_len) {
    return  pdf_write_real(self->value, out, out_len);
}


DLLEXPORT CosName* cos_name_new(PDF_TYPE_CODE_POINTS value, uint16_t value_len) {
    CosName* self = malloc(sizeof(CosName));
    self->type = COS_NODE_NAME;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = malloc(sizeof(PDF_TYPE_CODE_POINT) * value_len);
    memcpy(self->value, value, sizeof(PDF_TYPE_CODE_POINT) * value_len);
    self->value_len = value_len;
    return self;
 }

DLLEXPORT size_t cos_name_write(CosName* self, char* out, size_t out_len) {
    return  pdf_write_name(self->value, self->value_len, out, out_len);
}

DLLEXPORT CosLiteralStr* cos_literal_new(PDF_TYPE_STRING value, size_t value_len) {
    CosLiteralStr* self = malloc(sizeof(CosLiteralStr));
    self->type = COS_NODE_LIT_STR;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = malloc(sizeof(*value) * value_len);
    memcpy(self->value, value, sizeof(*value) * value_len);
    self->value_len = value_len;
    return self;
 }

DLLEXPORT size_t cos_literal_write(CosLiteralStr* self, char* out, size_t out_len) {
    return  pdf_write_literal(self->value, self->value_len, out, out_len);
}

DLLEXPORT CosHexString* cos_hex_string_new(PDF_TYPE_STRING value, size_t value_len) {
    CosHexString* self = malloc(sizeof(CosHexString));
    self->type = COS_NODE_HEX_STR;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = malloc(sizeof(*value) * value_len);
    memcpy(self->value, value, sizeof(*value) * value_len);
    self->value_len = value_len;
    return self;
 }

DLLEXPORT size_t cos_hex_string_write(CosHexString* self, char* out, size_t out_len) {
    return pdf_write_hex_string(self->value, self->value_len, out, out_len);
}

DLLEXPORT CosComment* cos_comment_new(PDF_TYPE_STRING value, size_t value_len) {
    CosComment* self = malloc(sizeof(CosComment));
    self->type = COS_NODE_COMMENT;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = malloc(sizeof(*value) * value_len);
    memcpy(self->value, value, sizeof(*value) * value_len);
    self->value_len = value_len;
    return self;
 }

DLLEXPORT size_t cos_comment_write(CosComment* self, char* out, size_t out_len, int indent) {
    size_t n = 0;
    for (; indent > 0; indent--) {
        if (n >= out_len) return 0;
        out[n++] = ' ';
    }
    return n + pdf_write_comment(self->value, self->value_len, out+n, out_len-n);
}

DLLEXPORT CosNull* cos_null_new(void) {
    CosNull* self = malloc(sizeof(CosNull));
    self->type = COS_NODE_NULL;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    return self;
}

DLLEXPORT size_t cos_null_write(CosNull* _self, char* out, size_t out_len) {
    strncpy(out, "null", out_len);
    return strnlen(out, out_len);
}

static CosOpCode _op(char *p, CosOpCode oc) {
    return *p ? COS_OP_Other : oc;
}

static CosOpCode _lookup_op_code(char *opn) {
    if (strlen(opn) < 4) {
        int dict = 0;
        char op[4] = {0, 0, 0, 0};
        char* p = op;
        strcpy(op, opn);

        switch (*(p++)) {
        case '"':     return _op(p, COS_OP_MoveSetShowText);
        case '\'':    return _op(p, COS_OP_MoveShowText);
        case 'B': switch (*(p++)) {
            case 0:   return COS_OP_FillStroke;
            case '*': return _op(p, COS_OP_EOFillStroke);
            case 'D': dict = 1; /* fallthrough */
            case 'M': return (*(p++) == 'C')
                    ? _op(p, dict ? COS_OP_BeginMarkedContentDict : COS_OP_BeginMarkedContent)
                    : COS_OP_Other;
            case 'I': return _op(p, COS_OP_BeginImage);
            case 'T': return _op(p, COS_OP_BeginText);
            case 'X': return _op(p, COS_OP_BeginExtended);
            default : return COS_OP_Other;
            }
        case 'C': return (*(p++) == 'S')
                ? _op(p, COS_OP_SetStrokeColorSpace)
                : COS_OP_Other;
        case 'D': switch (*(p++)) {
            case 'P': return _op(p, COS_OP_MarkPointDict);
            case 'o': return _op(p, COS_OP_XObject);
            default : return COS_OP_Other;
            }
        case 'E': switch (*(p++)) {
            case 'I': return _op(p, COS_OP_EndImage);
            case 'M': return (*(p++) == 'C')
                    ? _op(p, COS_OP_EndMarkedContent)
                    : COS_OP_Other;
            case 'T': return _op(p, COS_OP_EndText);
            case 'X': return _op(p, COS_OP_EndExtended);
            default : return COS_OP_Other;
            }
        case 'F': return _op(p, COS_OP_FillObsolete);
        case 'G': return _op(p, COS_OP_SetStrokeGray);
        case 'I': return (*(p++) == 'D')
                ? _op(p, COS_OP_ImageData)
                : COS_OP_Other;
        case 'J': return _op(p, COS_OP_SetLineCap);
        case 'K': return _op(p, COS_OP_SetStrokeCMYK);
        case 'M': switch (*(p++)) {
            case 0: return COS_OP_SetMiterLimit;
            case 'P': return _op(p, COS_OP_MarkPoint);
            default : return COS_OP_Other;
            }
        case 'Q': return _op(p, COS_OP_Restore);
        case 'R': return (*(p++) == 'G')
                ? _op(p, COS_OP_SetStrokeRGB)
                : COS_OP_Other;
        case 'S': switch (*(p++)) {
            case 0 :  return COS_OP_Stroke;
            case 'C': switch (*(p++)) {
                case 0 : return COS_OP_SetStrokeColor;
                case 'N' : return _op(p, COS_OP_SetStrokeColorN);
                default : return COS_OP_Other;
                }
            default : return COS_OP_Other;
            }
        case 'T':
            if (*(p+2)) return COS_OP_Other;
            switch (*(p++)) {
            case '*' : return COS_OP_TextNextLine;
            case 'D' : return COS_OP_TextMoveSet;
            case 'J' : return COS_OP_ShowSpaceText;
            case 'L' : return COS_OP_SetTextLeading;
            case 'c' : return COS_OP_SetCharSpacing;
            case 'd' : return COS_OP_TextMove;
            case 'f' : return COS_OP_SetFont;
            case 'j' : return COS_OP_ShowText;
            case 'm' : return COS_OP_SetTextMatrix;
            case 'r' : return COS_OP_SetTextRender;
            case 's' : return COS_OP_SetTextRise;
            case 'w' : return COS_OP_SetWordSpacing;
            case 'z' : return COS_OP_SetHorizScaling;
            default : return COS_OP_Other;
            }
        case 'W' : switch (*(p++)) {
            case 0: return COS_OP_Clip;
            case '*': return _op(p, COS_OP_EOClip);;
            default : return COS_OP_Other;
            }
        case 'b': switch (*(p++)) {
            case 0: return COS_OP_CloseFillStroke;
            case '*': return _op(p, COS_OP_CloseEOFillStroke);;
            default : return COS_OP_Other;
            }
        case 'c': switch (*(p++)) {
            case 0: return COS_OP_CurveTo;
            case 'm': return _op(p, COS_OP_ConcatMatrix);
            case 's': return _op(p, COS_OP_SetFillColorSpace);
            default : return COS_OP_Other;
            }
        case 'd': switch (*(p++)) {
            case 0: return COS_OP_SetDashPattern;
            case '0': return _op(p, COS_OP_SetCharWidth);
            case '1': return _op(p, COS_OP_SetCharWidthBBox);
            default : return COS_OP_Other;
            }
        case 'f':  switch (*(p++)) {
            case 0: return COS_OP_Fill;
            case '*': return _op(p, COS_OP_EOFill);
            default : return COS_OP_Other;
            }
        case 'g':  switch (*(p++)) {
            case 0: return COS_OP_SetFillGray;
            case 's': return _op(p, COS_OP_SetGraphicsState);
            default : return COS_OP_Other;
            }
        case 'h': return _op(p, COS_OP_ClosePath);
        case 'i': return _op(p, COS_OP_SetFlatness);
        case 'j': return _op(p, COS_OP_SetLineJoin);
        case 'k': return _op(p, COS_OP_SetFillCMYK);
        case 'l': return _op(p, COS_OP_LineTo);
        case 'm': return _op(p, COS_OP_MoveTo);
        case 'n': return _op(p, COS_OP_EndPath);
        case 'q': return _op(p, COS_OP_Save);
        case 'r':  switch (*(p++)) {
            case 'e': return _op(p, COS_OP_Rectangle);
            case 'g': return _op(p, COS_OP_SetFillRGB);
            case 'i': return _op(p, COS_OP_SetRenderingIntent);
            default : return COS_OP_Other;
            }
        case 's':  switch (*(p++)) {
            case 0: return COS_OP_CloseStroke;
            case 'c': return (*(p++) == 'n')
                    ? _op(p, COS_OP_SetFillColor)
                    : COS_OP_Other;
            case 'h': return _op(p, COS_OP_ShFill);
            default : return COS_OP_Other;
            }
        case 'v': return _op(p, COS_OP_CurveToInitial);
        case 'w': return _op(p, COS_OP_SetLineWidth);
        case 'y': return _op(p, COS_OP_CurveToFinal);
        default : return COS_OP_Other;
        }
    }

    return COS_OP_Other;
}

DLLEXPORT CosOp* cos_op_new(char* opn, int opn_len, CosNode** values, size_t elems) {
    size_t i;
    CosOp* self = malloc(sizeof(CosOp));
    self->type = COS_NODE_OP;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->opn = malloc(opn_len + 1);
    strncpy(self->opn, opn, opn_len);
    self->opn[opn_len] = 0;
    self->sub_type = _lookup_op_code(self->opn);
    self->elems = elems;
    self->values = malloc(sizeof(CosNode*) * elems);
    if (values) {
        for (i=0; i < elems; i++) {
            self->values[i] = values[i];
            cos_node_reference(values[i]);
        }
    }
    else {
        memset(self->values, 0, elems * sizeof(CosNode*));
    }

    return self;
}

static int _is_numeric(CosNode* self) {
    return self && (self->type == COS_NODE_REAL || self->type == COS_NODE_INT);
}

static int _is_stringy(CosNode* self) {
    return self && (self->type == COS_NODE_HEX_STR || self->type == COS_NODE_LIT_STR);
}

DLLEXPORT int cos_op_is_valid(CosOp* self) {
    if (!self || self->type != COS_NODE_OP) return 0;
    switch ((CosOpCode) self->sub_type) {
    case COS_OP_Other: case COS_OP_ImageData:
        return 0;

    case COS_OP_BeginImage: case COS_OP_EndImage:
    case COS_OP_BeginText:  case COS_OP_EndText:
    case COS_OP_EndMarkedContent:
    case COS_OP_BeginExtended: case COS_OP_EndExtended:
    case COS_OP_CloseEOFillStroke: case COS_OP_CloseFillStroke:
    case COS_OP_EOFillStroke: case COS_OP_FillStroke:
    case COS_OP_EOFill: case COS_OP_Fill: case COS_OP_FillObsolete:
    case COS_OP_ClosePath: case COS_OP_EndPath:
    case COS_OP_Save: case COS_OP_Restore:
    case COS_OP_CloseStroke: case COS_OP_Stroke:
    case COS_OP_TextNextLine: case COS_OP_EOClip: case COS_OP_Clip:
        return self->elems == 0;

    case COS_OP_SetFillColorSpace: case COS_OP_SetStrokeColorSpace:
    case COS_OP_XObject: case COS_OP_SetGraphicsState:
    case COS_OP_MarkPoint: case COS_OP_SetRenderingIntent:
    case COS_OP_ShFill:
    case COS_OP_BeginMarkedContent:
        return self->elems == 1 && self->values[0]->type == COS_NODE_NAME;

    case COS_OP_ShowText:
    case COS_OP_MoveShowText:
        return self->elems == 1 && _is_stringy(self->values[0]);

    case COS_OP_ShowSpaceText:
        return self->elems == 1
            && self->values[0]->type == COS_NODE_ARRAY;
        /* todo: check array elems */

    case COS_OP_SetFont:
        return self->elems == 2
            && self->values[0]->type == COS_NODE_NAME
            && _is_numeric(self->values[1]);

    case COS_OP_BeginMarkedContentDict:
    case COS_OP_MarkPointDict:
        return self->elems == 2
            && self->values[0]->type == COS_NODE_NAME
            && (self->values[1]->type == COS_NODE_DICT
                || self->values[1]->type == COS_NODE_NAME);

    case COS_OP_SetDashPattern:
        return self->elems == 2
            && self->values[0]->type == COS_NODE_ARRAY
            && _is_numeric(self->values[1]);
        /* todo: check array elems */

    case COS_OP_SetFlatness:
    case COS_OP_SetStrokeGray: case COS_OP_SetFillGray:
    case COS_OP_SetMiterLimit:
    case COS_OP_SetCharSpacing:
    case COS_OP_SetTextLeading:
    case COS_OP_SetTextRise:
    case COS_OP_SetWordSpacing: case COS_OP_SetHorizScaling:
    case COS_OP_SetLineWidth:
        return self->elems == 1 && _is_numeric(self->values[0]);

    case COS_OP_SetLineJoin: case COS_OP_SetLineCap:
    case COS_OP_SetTextRender:
        return self->elems == 1
            && self->values[0]->type == COS_NODE_INT
            && ((CosInt*)self->values[0])->value >= 0;

    case COS_OP_MoveTo: case COS_OP_LineTo:
    case COS_OP_TextMove: case COS_OP_TextMoveSet:
    case COS_OP_SetCharWidth:
        return self->elems == 2
            && _is_numeric(self->values[0])
            && _is_numeric(self->values[1]);

    case COS_OP_MoveSetShowText:
        return self->elems == 3
            && _is_numeric(self->values[0])
            && _is_numeric(self->values[1])
            && _is_stringy(self->values[2]);

    case COS_OP_SetFillRGB:
    case COS_OP_SetStrokeRGB:
        return self->elems == 3
            && _is_numeric(self->values[0])
            && _is_numeric(self->values[1])
            && _is_numeric(self->values[2]);

    case COS_OP_SetFillCMYK:
    case COS_OP_SetStrokeCMYK:
    case COS_OP_Rectangle:
    case COS_OP_CurveToInitial:
    case COS_OP_CurveToFinal:
        return self->elems == 4
            && _is_numeric(self->values[0])
            && _is_numeric(self->values[1])
            && _is_numeric(self->values[2])
            && _is_numeric(self->values[3]);

    case COS_OP_CurveTo:
    case COS_OP_ConcatMatrix:
    case COS_OP_SetCharWidthBBox:
    case COS_OP_SetTextMatrix:
        return self->elems == 6
            && _is_numeric(self->values[0])
            && _is_numeric(self->values[1])
            && _is_numeric(self->values[2])
            && _is_numeric(self->values[3])
            && _is_numeric(self->values[4])
            && _is_numeric(self->values[5]);

    case COS_OP_SetStrokeColor:
    case COS_OP_SetFillColor:
    case COS_OP_SetFillColorN:
    case COS_OP_SetStrokeColorN:
        /* these are context sensitive */
        return self->elems >= 1;
    }
    return 0;
}

DLLEXPORT size_t cos_op_write(CosOp* self, char* out, size_t out_len, int indent) {
    size_t n = 0;
    size_t i;
    size_t m;
    CosNode* comment = NULL;

    if (self->sub_type != COS_OP_EndImage) {
        int j;
        for (j = 0; j < indent; j++) {
            if (n >= out_len) return 0;
            out[n++] = ' ';
        }
    }

    for (i=0; i < self->elems; i++) {
        if (self->values[i]->type == COS_NODE_COMMENT) {
            comment = self->values[i];
        }
        else {
            int is_inline_image = self->values[i]->type == COS_NODE_INLINE_IMAGE;
            n += (m = _node_write(self->values[i], out+n, out_len - n, is_inline_image ? indent : 0));
            if (m == 0 ) return 0;
            if (n >= out_len) return 0;
            out[n++] = (is_inline_image ? '\n' : ' ');
        }
    }

    n += (m = _bufcat(out+n, out_len-n, self->opn));
    if (m == 0) return 0;

    if (comment && n < out_len) {
        n += _node_write(comment, out+n, out_len - n, 1);
        n--;
    }

    return n;
}

DLLEXPORT CosContent* cos_content_new(CosOp** values, size_t elems) {
    size_t i;
    CosContent* self = malloc(sizeof(CosContent));
    self->type = COS_NODE_CONTENT;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->elems = elems;
    self->values = malloc(sizeof(CosOp*) * elems);
    if (values) {
        for (i=0; i < elems; i++) {
            self->values[i] = values[i];
            cos_node_reference((CosNode*)values[i]);
        }
    }
    else {
        memset(self->values, 0, elems * sizeof(CosNode*));
    }

    return self;
}

static int _op_nesting(CosOp* op) {
    if (op->type == COS_NODE_OP) {
        switch (op->sub_type) {
        case COS_OP_Save:
        case COS_OP_BeginText:
        case COS_OP_BeginExtended:
        case COS_OP_BeginMarkedContent:
        case COS_OP_BeginMarkedContentDict:
            return 1;
        case COS_OP_Restore:
        case COS_OP_EndText:
        case COS_OP_EndExtended:
        case COS_OP_EndMarkedContent:
            return -1;
        default :
            break;
        }
    }
    return 0;
}

DLLEXPORT size_t cos_content_write(CosContent* self, char* out, size_t out_len) {
    size_t n = 0;
    size_t i;
    size_t m;
    int indent = 0; /* todo */

    for (i=0; i < self->elems; i++) {
        if (n >= out_len) return 0;
        CosOp* op = self->values[i];
        int ch = _op_nesting(op);

        if (i > 0 && n < out_len) out[n++] = '\n';
        if (ch < 0 && indent > 1) indent -= 2;
        n += (m = _node_write((CosNode*)op, out+n, out_len-n, indent));
        if (m == 0) return 0;
        if (out[n-1] == '\n') n--;
        if (ch > 0 && indent >= 0) indent += 2;

    }

    return n;
}

DLLEXPORT CosInlineImage* cos_inline_image_new(CosDict* dict, unsigned char* value, size_t value_len) {
    CosInlineImage* self = (void*) cos_stream_new(dict, value, value_len);
    self->type = COS_NODE_INLINE_IMAGE;
    self->check = COS_CHECK(self);
    return self;
}

DLLEXPORT size_t cos_inline_image_write(CosInlineImage* self, char* out, size_t out_len, int indent) {
    size_t n = 0, m, i;
    CosDict* dict = self->dict;

    for (; indent > 0; indent--) {
        if (n >= out_len) return 0;
        out[n++] = ' ';
    }

   for (i = 0; i < dict->elems; i++) {
        n += (m = _node_write((CosNode*)dict->keys[i], out+n, out_len - n, -1));
        if (m == 0 || n >= out_len) return 0;
        out[n++] = ' ';

        n += (m = _node_write(dict->values[i], out+n, out_len - n, -1));
        if (m == 0 || n >= out_len) return 0;
        out[n++] = ' ';
    }

    n += (m = _bufcat(out+n, out_len-n, "ID\n" ));
    if (m == 0 ||  self->value_len > out_len) return 0;
    memcpy(out+n, self->value, self->value_len);

    return n + self->value_len;
}

/* gives a modest overestimate of write buffer size for preallocation */
DLLEXPORT size_t cos_node_get_write_size(CosNode* self, int indent) {
    char out[64];
    size_t size = 0;
    size_t i;

    switch (self ? self->type : COS_NODE_NULL) {
    case COS_NODE_IND_OBJ:
        return 50 + cos_node_get_write_size(((CosIndObj*)self)->value, 0);
    case COS_NODE_INT:
        return snprintf(out, sizeof(out), "%" PRId64, ((CosInt*)self)->value);
    case COS_NODE_BOOL:
        return 5;
    case COS_NODE_NULL:
        return 4;
    case COS_NODE_REAL:
        return cos_real_write((CosReal*)self, out, sizeof(out));
    case COS_NODE_REF:
        return cos_ref_write((CosRef*)self, out, sizeof(out)) + 1;
    case COS_NODE_ARRAY:
    {
        CosArray* a = (void*)self;
        size += 4; /* '[ ' + ' ]' */
        for (i = 0; i < a->elems; i++) {
            size += cos_node_get_write_size(a->values[i], indent+2);
        }
        return size + (a->elems + 2) * (indent+1);
    }
    case COS_NODE_DICT:
    {
        CosDict* d = (void*)self;
        size += 6; /* '<< ' + ' >>' */
        for (i = 0; i < d->elems; i++) {
            size += cos_node_get_write_size((CosNode*)d->keys[i], indent);
            size += cos_node_get_write_size(d->values[i], indent+2);
            size += 2;
        }
        return size;
    }
    case COS_NODE_STREAM:
    case COS_NODE_INLINE_IMAGE:
    {
        struct CosStreamish *s = (void*) self;
        size += cos_node_get_write_size((CosNode*)s->dict, indent + 2);
        size += 20;
        if (s->value) size += s->value_len;
        return size;
    }
    case COS_NODE_NAME:
    {
        CosName* n = (void*) self;
        size++; /* leading '/' */
        for (i = 0; i < n->value_len; i++) {
            size += pdf_write_name_code(n->value[i], out, sizeof(out));
        }
        return size;
    }
    case COS_NODE_LIT_STR:
    {
        CosLiteralStr* ls = (void*) self;
        size += 2; /* '(' and ')' */
        for (i = 0; i < ls->value_len; i++) {
            switch(ls->value[i]) {
            case '\n': case '\r': case '\t': case '\f': case '\b':
            case '\\': case '(': case ')':
                /* escaped */
                size += 2;
                break;
            default:
                /* literal */
                size += 1;
            }
        }
        return size;
    }
    case COS_NODE_HEX_STR:
        return ((CosHexString*)self)->value_len * 2 + 2;

    case COS_NODE_OP:
    {
        CosOp* op = (void*) self;
        size = strlen(op->opn) + op->elems;
        for (i = 0; i < op->elems; i++) {
            size += 1 + cos_node_get_write_size(op->values[i], 0);
        }
        return size;
    }
    case COS_NODE_CONTENT:
    {
        CosContent* c = (void*) self;
        indent = 1;
        for (i = 0; i < c->elems; i++) {
            CosNode* node = (void*) c->values[i];
            if (node->type == COS_NODE_OP) {
                indent += 2 * _op_nesting((CosOp*)node);
                if (indent < 0) indent = 0;
            }
            size += indent;
            size += cos_node_get_write_size(node, indent);
            size++;
        }
        return size;
    }
    case COS_NODE_COMMENT:
    {
        size += 3;
        CosComment* c = (void*) self;
        for (i = 0; i < c->value_len; i++) {
            char ch = c->value[i];
            size += (ch == '\r' || ch == '\n') ? 3 : 1;
        }
        return size;
    }
    }

    return size;
}
