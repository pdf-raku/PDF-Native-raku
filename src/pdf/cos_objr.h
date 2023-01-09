#ifndef PDF_COS_OBJR_H_
#define PDF_COS_OBJR_H_

#include "pdf/cos.h"

struct _cos_objr {
    int32_t obj_num;
    int16_t gen_num;
    char* Type;
    cos_indref Pg;
    cos_indref Obj;
};

typedef struct _cos_objr *cos_objr;

DLLEXPORT int pdf_cos_objr_write(cos_objr, char*, int);

#endif
