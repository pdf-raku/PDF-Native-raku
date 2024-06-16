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
        fprintf(stderr, __FILE__ ":%d double done on %p\n", __LINE__, (void*) self);
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
        case COS_NODE_REF:
            n = cos_ref_write((CosRef*)self, out, out_len);
            break;
        case COS_NODE_ARRAY:
            n = cos_array_write((CosArray*)self, out, out_len);
            break;
        case COS_NODE_DICT:
            n = cos_dict_write((CosDict*)self, out, out_len);
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

DLLEXPORT size_t cos_array_write(CosArray* self, char* out, size_t out_len) {
    size_t n = 0;
    size_t i;
    if (out && out_len) {
        strncat(out, "[ ", out_len);
        n += 2;
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
    self->key_lens = (uint16_t*) malloc(elems);
    self->values = (CosNode**) malloc(sizeof(CosNode*) * elems);
    for (i=0; i < elems; i++) {
        self->keys[i] = malloc(key_lens[i] * sizeof(PDF_TYPE_CODE_POINTS));
        memcpy(self->keys[i], keys[i], key_lens[i] * sizeof(int32_t));
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
        strncat(out, "<< ", out_len);
        n += 3;
        for (i=0; i < self->elems; i++) {
            n += pdf_write_name(self->keys[i], self->key_lens[i], out+n, out_len-n);
            if (n < out_len) out[n++] = ' ';
            n += _node_write(self->values[i], out+n, out_len - n);
            if (n < out_len) out[n++] = ' ';
        }
        if (n < out_len) out[n++] = '>';
        if (n < out_len) out[n++] = '>';
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
    if (out && out_len) {
        if (self && self->obj_num > 0) {
            n = snprintf(out, out_len, "%ld %d R", self->obj_num, self->gen_num);
        }
        else {
            *out = 0;
        }
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

DLLEXPORT size_t cos_ind_obj_write(CosIndObj* self, char* out, size_t out_len) {
    size_t n = (size_t) snprintf(out, out_len, "%ld %d obj\n", self->obj_num, self->gen_num);
    n += _node_write(self->value, out+n, out_len-n);
    if (n < out_len) {
        strncat(out+n, "\nendobj\n", out_len-n);
        n += 8;
        if (n > out_len) n = out_len;
    }
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




