#include "pdf.h"
#include "pdf/cos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void _node_done(CosNode* self) {
    if (self) {
        switch (self->type) {
        case COS_NODE_BOOL:
        case COS_NODE_INT:
        case COS_NODE_NULL:
        case COS_NODE_REAL:
        case COS_NODE_REF:
            /* content does need freeing */
            break;
        case COS_NODE_IND_OBJ:
            _node_done(((CosIndObj*)self)->value);
            break;
        default:
            fprintf(stderr, __FILE__ ":%d type not yet handled: %d\n", __LINE__, self->type);
        }
        free((void*)self);
    }
}

DLLEXPORT void cos_fragment_done(CosNode* self) {
    _node_done(self);
}

static int _node_write(CosNode* self, char* out, int out_len) {
    int n = 0;
    if (self) {
        switch (self->type) {
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
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    return self;
}

DLLEXPORT int cos_ref_write(CosRef* self, char* out, int out_len) {
    if (self && out && out_len && self->obj_num > 0) {
        int n = snprintf(out, out_len, "%ld %d R", self->obj_num, self->gen_num);
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
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    self->value = value;
    return self;
}

DLLEXPORT int cos_ind_obj_write(CosIndObj* self, char* out, int out_len) {
    int n = cos_ref_write((CosRef*)self, out, out_len);
    strncat(out+n,"\n", out_len-n);
    n += _node_write(self->value, out+n+1, out_len-n-1);
    return n;
}

DLLEXPORT void cos_ind_obj_done(CosIndObj* self) {
    _node_done(self->value);
    free((void*)self);
}


