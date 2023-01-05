#ifndef PDF_COS_H_
#define PDF_COS_H_

#include "pdf.h"
#include <stdio.h>
#include <stdint.h>
#include <stdint.h>

struct cos_indobj {
    int32_t obj_num;
    int16_t gen_num;
};

typedef struct cos_indobj *cos_indref;


DLLEXPORT int pdf_cos_indref_write(cos_indref, char* out, int out_len);

#endif
