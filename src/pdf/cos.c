#include "pdf.h"
#include "pdf/cos.h"
#include "pdf/write.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DLLEXPORT void cos_node_done(CosNode* self) {
    if (self && --(self->ref) <= 0) {
        switch (self->type) {
        case COS_NODE_BOOL:
        case COS_NODE_INT:
        case COS_NODE_NULL:
        case COS_NODE_REAL:
        case COS_NODE_REF:
            /* leaf node */
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
        default:
            fprintf(stderr, __FILE__ ":%d type not yet handled: %d\n", __LINE__, self->type);
        }
    }
    return n;
}

DLLEXPORT CosRef* cos_ref_new(CosRef* self, uint64_t obj_num, uint32_t gen_num) {
    self = (CosRef*) malloc(sizeof(CosRef));
    self->type = COS_NODE_REF;
    self->ref = 1;
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    return self;
}

DLLEXPORT size_t cos_ref_write(CosRef* self, char* out, size_t out_len) {
    if (self && out && out_len && self->obj_num > 0) {
        size_t n = snprintf(out, out_len, "%ld %d R", self->obj_num, self->gen_num);
        return n >= out_len ? out_len - 1 : n;
    }
    else {
        if (out && out_len) *out = 0;
        return 0;
    }
}

DLLEXPORT CosIndObj* cos_ind_obj_new(CosIndObj* self, uint64_t obj_num, uint32_t gen_num, CosNode* value) {
    self = (CosIndObj*) malloc(sizeof(CosIndObj));
    self->type = COS_NODE_IND_OBJ;
    self->ref = 1;
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    self->value = value;
    value->ref++;
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
    self->ref = 1;
    self->value = value;
    return self;
}

DLLEXPORT size_t cos_int_write(CosInt* self, char* out, size_t out_len) {
    return  pdf_write_int(self->value, out, out_len);
}




