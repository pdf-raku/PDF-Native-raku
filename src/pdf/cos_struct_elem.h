#ifndef PDF_COS_STRUCT_ELEM_H_
#define PDF_COS_STRUCT_ELEM_H_

#include "pdf/cos_struct_ref.h"

struct _cos_struct_elem {
    int32_t obj_num;
    int16_t gen_num;
    char* Type;
    char* S;
    char* ID;
    cos_indref Pg;
    struct _cos_struct_elem *P;
    struct _cos_struct_ref *K;
};

typedef struct _cos_struct_elem *cos_struct_elem;

#endif
