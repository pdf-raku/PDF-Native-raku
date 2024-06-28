#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/write.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COS_CHECK(n) ((n)->type + 123)

DLLEXPORT void cos_node_reference(CosNode* self) {
    if (self == NULL) return;
    if (self->check != COS_CHECK(self)) {
        fprintf(stderr, __FILE__ ":%d node %p (type %d) not owned by us or corrupted\n", __LINE__, (void*) self, self->type);
    }
    self->ref_count++;
}

DLLEXPORT void cos_node_done(CosNode* self) {
    if (self == NULL) return;
    if (self->check != COS_CHECK(self)) {
        fprintf(stderr, __FILE__ ":%d node %p (type %d, ref %d) not owned by us or corrupted\n", __LINE__, (void*) self, self->type, self->ref_count);
    }
    else if (self->ref_count == 0) {
        fprintf(stderr, __FILE__ ":%d dead object\n", __LINE__);
    }
    else if (--(self->ref_count) <= 0) {
        switch (self->type) {
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
            free(((struct CosStringyNode*)self)->value);
            break;
        case COS_NODE_ARRAY:
            {
                size_t i;
                CosArray* a = (void*)self;
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
            {
                CosStream* stream = (CosStream*)self;
                cos_node_done((CosNode*)stream->dict);
                if (stream->value) free(stream->value);
            }
            break;
        default:
            fprintf(stderr, __FILE__ ":%d type not yet handled: %d\n", __LINE__, self->type);
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
    return self != NULL && (self->type == COS_NODE_LIT_STR || self->type == COS_NODE_HEX_STR);
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
        switch (self->type) {
            case COS_NODE_BOOL:
                return COS_CMP(((CosBool*)self)->value, ((CosBool*)obj)->value);
            case COS_NODE_INT:
                return COS_CMP(((CosInt*)self)->value, ((CosInt*)obj)->value);
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
                {
                    struct CosStringyNode* a = (void*)self;
                    struct CosStringyNode* b = (void*)obj;
                    return ((a->value_len == b->value_len) && !_cmp_chars(a->value, b->value, a->value_len))
                        ? COS_CMP_EQUAL
                        : COS_CMP_DIFFERENT;
                }
            case COS_NODE_ARRAY:
                {
                    size_t i;
                    CosArray* a = (void*)self;
                    CosArray* b = (void*)obj;
                    int rv = COS_CMP_EQUAL;
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
                break;
            case COS_NODE_DICT:
                {
                    size_t i;
                    CosDict* a = (void*)self;
                    CosDict* b = (void*)obj;
                    int rv = COS_CMP_EQUAL;
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
                {
                    CosStream* a = (void*)self;
                    CosStream* b = (void*)obj;
                    int rv = COS_CMP_EQUAL;
                    if (a->value_len != b->value_len || _cmp_chars(a->value, b->value, a->value_len)) {
                        rv = COS_CMP_DIFFERENT;
                    }
                    else {
                        rv = cos_node_cmp((CosNode*)a->dict, (CosNode*)b->dict);
                        if (rv > COS_CMP_DIFFERENT) rv = COS_CMP_DIFFERENT;
                    }
                    return rv;
                }
            default:
                fprintf(stderr, __FILE__ ":%d type not yet handled: %d\n", __LINE__, self->type);
                return COS_CMP_EQUAL;
        }
    }
}

static int _node_write(CosNode* self, char* out, int out_len, int indent) {
    int n = 0;

    switch (self ? self->type : COS_NODE_NULL) {
    case COS_NODE_IND_OBJ:
        n = cos_ind_obj_write((CosIndObj*)self, out, out_len);
        break;
    case COS_NODE_INT:
        n = cos_int_write((CosInt*)self, out, out_len);
        break;
    case COS_NODE_BOOL:
        n = cos_bool_write((CosBool*)self, out, out_len);
        break;
    case COS_NODE_NULL:
        n = cos_null_write((CosNull*)self, out, out_len);
        break;
    case COS_NODE_REAL:
        n = cos_real_write((CosReal*)self, out, out_len);
        break;
    case COS_NODE_REF:
        n = cos_ref_write((CosRef*)self, out, out_len);
        break;
    case COS_NODE_ARRAY:
        n = cos_array_write((CosArray*)self, out, out_len, indent);
        break;
    case COS_NODE_DICT:
        n = cos_dict_write((CosDict*)self, out, out_len, indent);
        break;
    case COS_NODE_NAME:
        n = cos_name_write((CosName*)self, out, out_len);
        break;
    case COS_NODE_LIT_STR:
        n = cos_literal_write((CosLiteralStr*)self, out, out_len);
        break;
    case COS_NODE_HEX_STR:
        n = cos_hex_string_write((CosHexString*)self, out, out_len);
        break;
    case COS_NODE_STREAM:
        n = cos_stream_write((CosStream*)self, out, out_len);
        break;
    default:
        fprintf(stderr, __FILE__ ":%d type not yet handled: %d\n", __LINE__, self->type);
    }

    return n;
}

DLLEXPORT CosArray* cos_array_new(CosArray* self, CosNode** values, size_t elems) {
    size_t i;
    self = malloc(sizeof(CosArray));
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

static int _bufcat(char *in, char* out, int out_len) {
    int n;
    for (n=0; in[n] && out_len > 0; n++) {
        out[n] = in[n];
    }
    return n;
}

DLLEXPORT size_t cos_array_write(CosArray* self, char* out, size_t out_len, int indent) {
    size_t n = 0;
    size_t i;
    size_t m;

    if (!out || out_len < 2) return 0;

    n += _bufcat("[ ", out, out_len);
    for (i=0; i < self->elems; i++) {
        n += (m = _node_write(self->values[i], out+n, out_len - n, indent));
        if (m == 0 || n >= out_len) return 0;
        out[n++] = ' ';
    }
    if (n >= out_len) return 0;
    out[n++] = ']';

    return n;
}

DLLEXPORT CosDict* cos_dict_new(CosDict* self, CosName** keys, CosNode** values, size_t elems) {
    size_t i;
    self = malloc(sizeof(CosDict));
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

    if (!index) index = cos_dict_build_index(self);

    for (low = 0, high = self->index_len;;) {
        size_t mid = low + (high - low) / 2;
        CosName* this_key = self->keys[ index[mid] ];
        int cmp = _cmp_names(key, this_key); 

        if (cmp == 0) {
            return self->values[ index[mid] ];
        }
        else if (low == high) {
            break;
        }
        else if (cmp < 0) {
            high = mid -1;
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

/* Same as PDF::IO::Writer::MultiLineDictSize constant */
#define MultiLineDictSize 65

DLLEXPORT size_t cos_dict_write(CosDict* self, char* out, size_t out_len, int indent) {
    size_t n = 0;
    size_t i, m;
    size_t* pos = malloc((self->elems+1) * sizeof(size_t));
    int elem_indent = indent >= 0 ? indent + 2 : -1;

    if (!out || out_len <= 4) goto bail;

    n += _bufcat("<< ", out, out_len);

    for (i = 0; i < self->elems; i++) {
        pos[i] = n - 1;

        n += (m = _node_write((CosNode*)self->keys[i], out+n, out_len - n, 0));
        if (m == 0 || n >= out_len) goto bail;
        out[n++] = ' ';

        n += (m = _node_write(self->values[i], out+n, out_len - n, elem_indent));
        if (m == 0 || n >= out_len) goto bail;
        out[n++] = ' ';
    }

    if (elem_indent > 0 && n - self->elems -3 >= MultiLineDictSize) {
        int m;

        pos[self->elems] = n;
        n += (m = _indent_items(self, out, out_len, pos, elem_indent));
        if (m == 0 || n + indent >= out_len) goto bail;

        out[n-1] = '\n';
        for (; indent > 0; indent--) {
            if (n >= out_len) goto bail;
            out[n++] = ' ';
        }
    }

    if (n+1 >= out_len) goto bail;
    n += _bufcat(">>", out+n, out_len-n);

    free(pos);
    return n;

bail:
    free(pos);
    return 0;
}

DLLEXPORT CosRef* cos_ref_new(CosRef* self, uint64_t obj_num, uint32_t gen_num) {
    self = malloc(sizeof(CosRef));
    self->type = COS_NODE_REF;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    return self;
}

DLLEXPORT size_t cos_ref_write(CosRef* self, char* out, size_t out_len) {
    return snprintf(out, out_len, "%ld %d R", self->obj_num, self->gen_num);
}

DLLEXPORT CosIndObj* cos_ind_obj_new(CosIndObj* self, uint64_t obj_num, uint32_t gen_num, CosNode* value) {
    self = malloc(sizeof(CosIndObj));
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
    n = snprintf(out, out_len, "%ld %d obj\n", self->obj_num, self->gen_num);
    n += _node_write(self->value, out+n, out_len-n, 0);
    if (n + 7 > out_len) return 0;
    n += _bufcat("\nendobj", out+n, out_len-n);
    return n;
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
                struct CosArrayishNode* a = (void*)self;
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
        default:
            break;
    }
}

DLLEXPORT CosCryptNodeCtx* cos_crypt_ctx_new(CosCryptNodeCtx* self, CosCryptFunc crypt_cb, CosCryptMode mode, unsigned char* key, int key_len) {
    self = malloc(sizeof(CosCryptNodeCtx));

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

DLLEXPORT CosStream* cos_stream_new(CosStream* self, CosDict* dict, unsigned char* value, size_t value_len) {
    self = malloc(sizeof(CosStream));
    self->type = COS_NODE_STREAM;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->dict = dict;
    cos_node_reference((CosNode*)dict);

    if (value && value_len) {
        self->value = malloc(value_len);
        memcpy(self->value, value, value_len);
        self->value_len = value_len;
    }
    else {
        self->value = NULL;
        self->value_len = 0;
    }

    return self;
}

DLLEXPORT size_t cos_stream_write(CosStream* self, char* out, size_t out_len) {
    size_t n = cos_dict_write(self->dict, out, out_len, 0);
    size_t i;
    n += _bufcat("\nstream\n", out+n, out_len-n);
    if (self->value) {
        for (i = 0; i < self->value_len && n < out_len; i++) {
            out[n++] = self->value[i];
        }
    }
    if (n + 10 > out_len) return 0;
    n += _bufcat("\nendstream", out+n, out_len-n);
    return n;
}

DLLEXPORT CosInt* cos_int_new(CosInt* self, PDF_TYPE_INT value) {
    self = malloc(sizeof(CosInt));
    self->type = COS_NODE_INT;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_int_write(CosInt* self, char* out, size_t out_len) {
    return  pdf_write_int(self->value, out, out_len);
}

DLLEXPORT CosBool* cos_bool_new(CosBool* self, PDF_TYPE_BOOL value) {
    self = malloc(sizeof(CosBool));
    self->type = COS_NODE_BOOL;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_bool_write(CosBool* self, char* out, size_t out_len) {
    return  pdf_write_bool(self->value, out, out_len);
}

DLLEXPORT CosReal* cos_real_new(CosReal* self, PDF_TYPE_REAL value) {
    self = malloc(sizeof(CosReal));
    self->type = COS_NODE_REAL;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_real_write(CosReal* self, char* out, size_t out_len) {
    return  pdf_write_real(self->value, out, out_len);
}


DLLEXPORT CosName* cos_name_new(CosName* self, PDF_TYPE_CODE_POINTS value, uint16_t value_len) {
    self = malloc(sizeof(CosName));
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

DLLEXPORT CosLiteralStr* cos_literal_new(CosLiteralStr* self, PDF_TYPE_STRING value, size_t value_len) {
    self = malloc(sizeof(CosLiteralStr));
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

DLLEXPORT CosHexString* cos_hex_string_new(CosHexString* self, PDF_TYPE_STRING value, size_t value_len) {
    self = malloc(sizeof(CosHexString));
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

DLLEXPORT CosNull* cos_null_new(CosNull* self) {
    self = malloc(sizeof(CosNull));
    self->type = COS_NODE_NULL;
    self->check = COS_CHECK(self);
    self->ref_count = 1;
    return self;
}

DLLEXPORT size_t cos_null_write(CosNull* self, char* out, size_t out_len) {
    strncpy(out, "null", out_len);
    return strnlen(out, out_len);
}

