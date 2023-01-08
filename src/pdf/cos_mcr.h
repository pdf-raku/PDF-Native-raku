#ifndef PDF_COS_MCR_H_
#define PDF_COS_MCR_H_

#include "pdf/cos.h"

struct _cos_mcr {
    int32_t obj_num;
    int16_t gen_num;
    char* Type;
    char* S;
    char* ID;
    cos_indref Pg;
    cos_indref Stm;
    int32_t MCID;
};

typedef struct _cos_mcr *cos_mcr;

#endif
