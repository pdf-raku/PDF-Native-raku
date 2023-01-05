#include "cos.h"

DLLEXPORT int pdf_cos_indref_write(cos_indref ind_ref, char* out, int out_len) {
    if (out && out_len && ind_ref && ind_ref->obj_num > 0) {
        int n = snprintf(out, out_len, "%d %d R", ind_ref->obj_num, ind_ref->gen_num);
        return n >= out_len ? out_len - 1 : n;
    }
    else {
        return 0;
    }
}
