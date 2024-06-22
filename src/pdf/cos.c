#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/write.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DLLEXPORT void cos_node_reference(CosNode* self) {
    if (self->check != self->type + 123) {
        fprintf(stderr, __FILE__ ":%d node %p (type %d) not owned by us or corrupted\n", __LINE__, (void*) self, self->type);
    }
    self->ref_count++;
}

DLLEXPORT void cos_node_done(CosNode* self) {
    if (self == NULL) return;
    if (self->check != self->type + 123) {
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
        case COS_NODE_LITERAL:
        case COS_NODE_HEX:
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

static int _cmp_code_points(PDF_TYPE_CODE_POINTS v1, PDF_TYPE_CODE_POINTS v2, uint16_t key_len) {
    uint16_t i;
    for (i = 0; i < key_len; i++) {
        if (v1[i] != v2[i]) return COS_CMP_DIFFERENT;
    }
    return COS_CMP_EQUAL;
}

static int _cmp_bytes(unsigned char* v1, unsigned char* v2, uint16_t key_len) {
    uint16_t i;
    for (i = 0; i < key_len; i++) {
        if (v1[i] != v2[i]) return COS_CMP_DIFFERENT;
    }
    return COS_CMP_EQUAL;
}

static int _cmp_chars(char* v1, char* v2, uint16_t key_len) {
    uint16_t i;
    for (i = 0; i < key_len; i++) {
        if (v1[i] != v2[i]) return COS_CMP_DIFFERENT;
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
    return self != NULL && (self->type == COS_NODE_LITERAL || self->type == COS_NODE_HEX);
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
                ? COS_CMP_SLIGHTLY_DIFFERENT
                : COS_CMP_DIFFERENT;
        }
        else if (_is_stringy_node(self) && _is_stringy_node(obj)) {
            struct CosStringyNode* a = (void*)self;
            struct CosStringyNode* b = (void*)obj;
            if (a->value_len == b->value_len
                && _cmp_chars(a->value, b->value, a->value_len) == COS_CMP_EQUAL) {
                return COS_CMP_SLIGHTLY_DIFFERENT;
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
            case COS_NODE_LITERAL:
            case COS_NODE_HEX:
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
                        if (cmp == COS_CMP_SLIGHTLY_DIFFERENT) {
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
                    if (a->elems != b->elems) return COS_CMP_DIFFERENT;
                    for (i = 0; i < a->elems; i++) {
                        if (cos_node_cmp((CosNode*)a->keys[i], (CosNode*)b->keys[i])) return COS_CMP_DIFFERENT;
                        {
                            int cmp = cos_node_cmp(a->values[i], b->values[i]);
                            if (cmp == COS_CMP_SLIGHTLY_DIFFERENT) {
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
                    if (a->value_len != b->value_len || _cmp_bytes(a->value, b->value, a->value_len)) {
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

static int _node_write(CosNode* self, char* out, int out_len) {
    int n = 0;
    if (self) {
        switch (self->type) {
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
            n = cos_array_write((CosArray*)self, out, out_len);
            break;
        case COS_NODE_DICT:
            n = cos_dict_write((CosDict*)self, out, out_len);
            break;
        case COS_NODE_NAME:
            n = cos_name_write((CosName*)self, out, out_len);
            break;
        case COS_NODE_LITERAL:
            n = cos_literal_write((CosLiteral*)self, out, out_len);
            break;
        case COS_NODE_HEX:
            n = cos_hex_string_write((CosHexString*)self, out, out_len);
            break;
        case COS_NODE_STREAM:
            n = cos_stream_write((CosStream*)self, out, out_len);
            break;
        default:
            fprintf(stderr, __FILE__ ":%d type not yet handled: %d\n", __LINE__, self->type);
        }
    }
    return n;
}

DLLEXPORT CosArray* cos_array_new(CosArray* self, CosNode** values, size_t elems) {
    size_t i;
    self = malloc(sizeof(CosArray));
    self->type = COS_NODE_ARRAY;
    self->check = self->type + 123;
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

DLLEXPORT size_t cos_array_write(CosArray* self, char* out, size_t out_len) {
    size_t n = 0;
    size_t i;
    if (out && out_len) {
        n += _bufcat("[ ", out, out_len);
        for (i=0; i < self->elems; i++) {
            n += _node_write(self->values[i], out+n, out_len - n);
            if (n < out_len) out[n++] = ' ';
        }
        if (n < out_len) out[n++] = ']';
    }
    return n;
}

DLLEXPORT CosDict* cos_dict_new(CosDict* self, CosName** keys, CosNode** values, size_t elems) {
    size_t i;
    self = malloc(sizeof(CosDict));
    self->type = COS_NODE_DICT;
    self->check = self->type + 123;
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
    return self;
}

DLLEXPORT CosNode* cos_dict_lookup(CosDict* self, PDF_TYPE_CODE_POINTS key, uint16_t key_len) {
    size_t i;

    for (i = 0; i < self->elems; i++) {
        if (self->keys[i]->value_len == key_len) {
            if (_cmp_code_points(key, self->keys[i]->value, self->keys[i]->value_len) == 0) {
                //* 32000-2 7.3.7: A null value implies a non-existent entry
                if (self->values[i]->type != COS_NODE_NULL) {
                    return self->values[i];
                }
            }
        }
    }
    return NULL;
}

DLLEXPORT size_t cos_dict_write(CosDict* self, char* out, size_t out_len) {
    size_t n = 0;
    size_t i;
    if (out && out_len) {
        n += _bufcat("<< ", out, out_len);
        for (i=0; i < self->elems; i++) {
            n += _node_write((CosNode*)self->keys[i], out+n, out_len - n);
            if (n < out_len) out[n++] = ' ';
            n += _node_write(self->values[i], out+n, out_len - n);
            if (n < out_len) out[n++] = ' ';
        }
        n += _bufcat(">>", out+n, out_len-n);
    }
    return n;
}

DLLEXPORT CosRef* cos_ref_new(CosRef* self, uint64_t obj_num, uint32_t gen_num) {
    self = malloc(sizeof(CosRef));
    self->type = COS_NODE_REF;
    self->check = self->type + 123;
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
    self->check = self->type + 123;
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
    n += _node_write(self->value, out+n, out_len-n);
    n += _bufcat("\nendobj", out+n, out_len-n);
    return n;
}

typedef struct {
    unsigned char *key;
    int key_len;

    uint64_t obj_num;
    uint32_t gen_num;

    unsigned char *buf;
    size_t   buf_len;
} CosCryptNodeCtx;

typedef void (*CosCryptFunc) (CosCryptNodeCtx*, PDF_TYPE_STRING, size_t);

static void _crypt_node(CosNode* self, CosCryptNodeCtx* crypt_ctx, CosCryptFunc crypt_cb) {
    switch (self->type) {
        case COS_NODE_LITERAL:
        case COS_NODE_HEX:
            {
                struct CosStringyNode* s = (struct CosStringyNode*) self;
                crypt_cb(crypt_ctx, s->value, s->value_len);
            }
            break;
        case COS_NODE_ARRAY:
        case COS_NODE_DICT:
            {
                size_t i;
                struct CosArrayishNode* a = (struct CosArrayishNode*)self;
                for (i=0; i < a->elems; i++) {
                    _crypt_node(a->values[i], crypt_ctx, crypt_cb);
                }
            }
            break;
        default:
            break;
    }
}

DLLEXPORT void cos_ind_obj_crypt(CosIndObj* self, unsigned char* key, int key_len, CosCryptFunc crypt_cb) {
    CosCryptNodeCtx crypt_ctx = {
        key, key_len,
        self->obj_num, self->gen_num,
        malloc(512), 512
    };
    _crypt_node(self->value, &crypt_ctx, crypt_cb);
    free(crypt_ctx.buf);
}

DLLEXPORT CosStream* cos_stream_new(CosStream* self, CosDict* dict, unsigned char* value, size_t value_len) {
    self = malloc(sizeof(CosStream));
    self->type = COS_NODE_STREAM;
    self->check = self->type + 123;
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
    size_t n = cos_dict_write(self->dict, out, out_len);
    size_t i;
    n += _bufcat("\nstream\n", out+n, out_len-n);
    if (self->value) {
        for (i = 0; i < self->value_len && n < out_len; i++) {
            out[n++] = self->value[i];
        }
    }
    n += _bufcat("\nendstream", out+n, out_len-n);
    return n;
}

DLLEXPORT CosInt* cos_int_new(CosInt* self, PDF_TYPE_INT value) {
    self = malloc(sizeof(CosInt));
    self->type = COS_NODE_INT;
    self->check = self->type + 123;
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
    self->check = self->type + 123;
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
    self->check = self->type + 123;
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
    self->check = self->type + 123;
    self->ref_count = 1;
    self->value = malloc(sizeof(PDF_TYPE_CODE_POINT) * value_len);
    memcpy(self->value, value, sizeof(PDF_TYPE_CODE_POINT) * value_len);
    self->value_len = value_len;
    return self;
 }

DLLEXPORT size_t cos_name_write(CosName* self, char* out, size_t out_len) {
    return  pdf_write_name(self->value, self->value_len, out, out_len);
}

DLLEXPORT CosLiteral* cos_literal_new(CosLiteral* self, PDF_TYPE_STRING value, size_t value_len) {
    self = malloc(sizeof(CosLiteral));
    self->type = COS_NODE_LITERAL;
    self->check = self->type + 123;
    self->ref_count = 1;
    self->value = malloc(sizeof(*value) * value_len);
    memcpy(self->value, value, sizeof(*value) * value_len);
    self->value_len = value_len;
    return self;
 }

DLLEXPORT size_t cos_literal_write(CosLiteral* self, char* out, size_t out_len) {
    return  pdf_write_literal(self->value, self->value_len, out, out_len);
}

DLLEXPORT CosHexString* cos_hex_string_new(CosHexString* self, PDF_TYPE_STRING value, size_t value_len) {
    self = malloc(sizeof(CosHexString));
    self->type = COS_NODE_HEX;
    self->check = self->type + 123;
    self->ref_count = 1;
    self->value = malloc(sizeof(*value) * value_len);
    memcpy(self->value, value, sizeof(*value) * value_len);
    self->value_len = value_len;
    return self;
 }

DLLEXPORT size_t cos_hex_string_write(CosHexString* self, char* out, size_t out_len) {
    return  pdf_write_hex_string(self->value, self->value_len, out, out_len);
}

DLLEXPORT CosNull* cos_null_new(CosNull* self) {
    self = malloc(sizeof(CosNull));
    self->type = COS_NODE_NULL;
    self->check = self->type + 123;
    self->ref_count = 1;
    return self;
}

DLLEXPORT size_t cos_null_write(CosNull* self, char* out, size_t out_len) {
    strncpy(out, "null", out_len);
    return strnlen(out, out_len);
}

