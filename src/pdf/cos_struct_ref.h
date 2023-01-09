#ifndef PDF_COS_STRUCT_REF_H_
#define PDF_COS_STRUCT_REF_H_

#include "pdf/cos_mcr.h"
#include "pdf/cos_objr.h"
#include "pdf/cos_struct_elem.h"

union cos_struct_ref {
    int32_t  mcid;
    cos_mcr  mcr;
    cos_objr objr;
    struct _cos_struct_elem *elem;
};

enum cos_struct_reftype {
    COS_STRUCT_REF_ELEM,  /* reference to child struct elem */
    COS_STRUCT_REF_MCID,  /* direct reference to content stream */
    COS_STRUCT_REF_OBJR,  /* reference via cos_objr struct */
    COS_STRUCT_REF_MCR    /* reference via cos_mcr struct */
};

struct _cos_struct_ref {
    enum cos_struct_reftype type;
    union cos_struct_ref ref;
};

#endif
