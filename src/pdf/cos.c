#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/write.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DLLEXPORT void cos_node_reference(CosNode* self) {
    self->ref_count++;
}

DLLEXPORT void cos_node_done(CosNode* self) {
    if (!self || self->ref_count == 0) {
        return;
    }
    if (--(self->ref_count) <= 0) {
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
                CosArray* a = (CosArray*)self;
                for (i=0; i < a->elems; i++) {
                    cos_node_done(a->values[i]);
                }
                free(a->values);
            }
            break;
        case COS_NODE_DICT:
             {
                size_t i;
                CosDict* a = (CosDict*)self;
                for (i=0; i < a->elems; i++) {
                    free(a->keys[i]);
                    cos_node_done(a->values[i]);
                }
                free(a->keys);
                free(a->key_lens);
                free(a->values);
            }
            break;
        case COS_NODE_IND_OBJ:
            cos_node_done(((CosIndObj*)self)->value);
            break;
        default:
            fprintf(stderr, __FILE__ ":%d type not yet handled: %d\n", __LINE__, self->type);
        }
        free((void*)self);
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
        default:
            fprintf(stderr, __FILE__ ":%d type not yet handled: %d\n", __LINE__, self->type);
        }
    }
    return n;
}

DLLEXPORT CosArray* cos_array_new(CosArray* self, CosNode** values, size_t elems) {
    size_t i;
    self = (CosArray*) malloc(sizeof(CosArray));
    self->type = COS_NODE_ARRAY;
    self->ref_count = 1;
    self->elems = elems;
    self->values = (CosNode**) malloc(sizeof(CosNode*) * elems);
    for (i=0; i < elems; i++) {
        self->values[i] = values[i];
        values[i]->ref_count++;
    }
    return self;
}

static int _bufcat(char* out, int out_len, char *in) {
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
        n += _bufcat(out, out_len, "[ ");
        for (i=0; i < self->elems; i++) {
            n += _node_write(self->values[i], out+n, out_len - n);
            if (n < out_len) out[n++] = ' ';
        }
        if (n < out_len) out[n++] = ']';
    }
    return n;
}

DLLEXPORT CosDict* cos_dict_new(CosDict* self, PDF_TYPE_CODE_POINTS* keys, CosNode** values, uint16_t* key_lens, size_t elems) {
    size_t i;
    self = (CosDict*) malloc(sizeof(CosDict));
    self->type = COS_NODE_DICT;
    self->ref_count = 1;
    self->elems = elems;
    self->keys = (PDF_TYPE_CODE_POINTS*) malloc(sizeof(PDF_TYPE_CODE_POINTS) * elems);
    self->key_lens = (uint16_t*) malloc(elems * sizeof(uint16_t));
    self->values = (CosNode**) malloc(sizeof(CosNode*) * elems);
    for (i=0; i < elems; i++) {
        self->keys[i] = malloc(key_lens[i] * sizeof(PDF_TYPE_CODE_POINT));
        memcpy(self->keys[i], keys[i], key_lens[i] * sizeof(PDF_TYPE_CODE_POINT));
        self->key_lens[i] = key_lens[i];
        self->values[i] = values[i];
        values[i]->ref_count++;
    }
    return self;
}

static int _cmp_code_points(PDF_TYPE_CODE_POINTS v1, PDF_TYPE_CODE_POINTS v2, uint16_t key_len) {
    uint16_t i;
    for (i = 0; i < key_len; i++) {
        if (v1[i] != v2[i]) {
            return v1[i] > v2[i] ? 1 : -1;
        }
    }
    return 0;
}

DLLEXPORT CosNode* cos_dict_lookup(CosDict* self, PDF_TYPE_CODE_POINTS key, uint16_t key_len) {
    size_t i;

    for (i = 0; i < self->elems; i++) {
        if (self->key_lens[i] == key_len) {
            if (_cmp_code_points(key, self->keys[i], key_len) == 0) {
                return self->values[i];
            }
        }
    }
    return NULL;
}

DLLEXPORT size_t cos_dict_write(CosDict* self, char* out, size_t out_len) {
    size_t n = 0;
    size_t i;
    if (out && out_len) {
        n += _bufcat(out, out_len, "<< ");
        for (i=0; i < self->elems; i++) {
            n += pdf_write_name(self->keys[i], self->key_lens[i], out+n, out_len-n);
            if (n < out_len) out[n++] = ' ';
            n += _node_write(self->values[i], out+n, out_len - n);
            if (n < out_len) out[n++] = ' ';
        }
        n += _bufcat(out+n, out_len-n, ">>");
    }
    return n;
}

DLLEXPORT CosRef* cos_ref_new(CosRef* self, uint64_t obj_num, uint32_t gen_num) {
    self = (CosRef*) malloc(sizeof(CosRef));
    self->type = COS_NODE_REF;
    self->ref_count = 1;
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    return self;
}

DLLEXPORT size_t cos_ref_write(CosRef* self, char* out, size_t out_len) {
    size_t n = 0;
    if (out && out_len && self->obj_num > 0) {
        n = snprintf(out, out_len, "%ld %d R", self->obj_num, self->gen_num);
    }
    return n;
}

DLLEXPORT CosIndObj* cos_ind_obj_new(CosIndObj* self, uint64_t obj_num, uint32_t gen_num, CosNode* value) {
    self = (CosIndObj*) malloc(sizeof(CosIndObj));
    self->type = COS_NODE_IND_OBJ;
    self->ref_count = 1;
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    self->value = value;
    value->ref_count++;
    return self;
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

DLLEXPORT size_t cos_ind_obj_write(CosIndObj* self, char* out, size_t out_len) {
    size_t n = (size_t) snprintf(out, out_len, "%ld %d obj\n", self->obj_num, self->gen_num);
    n += _node_write(self->value, out+n, out_len-n);
    n += _bufcat(out+n, out_len-n, "\nendobj\n");
    return n;
}

DLLEXPORT CosInt* cos_int_new(CosInt* self, PDF_TYPE_INT value) {
    self = (CosInt*) malloc(sizeof(CosInt));
    self->type = COS_NODE_INT;
    self->ref_count = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_int_write(CosInt* self, char* out, size_t out_len) {
    return  pdf_write_int(self->value, out, out_len);
}

DLLEXPORT CosBool* cos_bool_new(CosBool* self, PDF_TYPE_BOOL value) {
    self = (CosBool*) malloc(sizeof(CosBool));
    self->type = COS_NODE_BOOL;
    self->ref_count = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_bool_write(CosBool* self, char* out, size_t out_len) {
    return  pdf_write_bool(self->value, out, out_len);
}

DLLEXPORT CosReal* cos_real_new(CosReal* self, PDF_TYPE_REAL value) {
    self = (CosReal*) malloc(sizeof(CosReal));
    self->type = COS_NODE_REAL;
    self->ref_count = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_real_write(CosReal* self, char* out, size_t out_len) {
    return  pdf_write_real(self->value, out, out_len);
}


DLLEXPORT CosName* cos_name_new(CosName* self, PDF_TYPE_CODE_POINTS value, uint16_t value_len) {
    self = (CosName*) malloc(sizeof(CosName));
    self->type = COS_NODE_NAME;
    self->ref_count = 1;
    self->value = (PDF_TYPE_CODE_POINTS) malloc(sizeof(PDF_TYPE_CODE_POINT) * value_len);
    memcpy(self->value, value, sizeof(PDF_TYPE_CODE_POINT) * value_len);
    self->value_len = value_len;
    return self;
 }

DLLEXPORT size_t cos_name_write(CosName* self, char* out, size_t out_len) {
    return  pdf_write_name(self->value, self->value_len, out, out_len);
}

DLLEXPORT CosLiteral* cos_literal_new(CosLiteral* self, PDF_TYPE_STRING value, size_t value_len) {
    self = (CosLiteral*) malloc(sizeof(CosLiteral));
    self->type = COS_NODE_LITERAL;
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
    self = (CosHexString*) malloc(sizeof(CosHexString));
    self->type = COS_NODE_HEX;
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
    self = (CosNull*) malloc(sizeof(CosNull));
    self->type = COS_NODE_NULL;
    self->ref_count = 1;
    return self;
}

DLLEXPORT size_t cos_null_write(CosNull* self, char* out, size_t out_len) {
    strncpy(out, "null", out_len);
    return strnlen(out, out_len);
}

