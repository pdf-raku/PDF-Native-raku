#include "pdf.h"
#include "pdf/cos.h"
#include <stdio.h>
#include <stdlib.h>

DLLEXPORT CosRef* cos_ref_new(CosRef* self, uint64_t obj_num, uint32_t gen_num) {
    self = (CosRef*) malloc(sizeof(CosRef));
    self->type = COS_NODE_REF;
    self->obj_num = obj_num;
    self->gen_num = gen_num;
    return self;
}

DLLEXPORT void cos_ref_done(CosRef* self) {
    free((void*)self);
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
