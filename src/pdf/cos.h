#ifndef PDF_COS_H_
#define PDF_COS_H_

#include <stddef.h>
#include <stdint.h>
#include "pdf/types.h"

typedef enum COS_NODE_TYPE {
    COS_NODE_ANY,
    COS_NODE_ARRAY,
    COS_NODE_BOOL,
    COS_NODE_DICT,
    COS_NODE_HEX,
    COS_NODE_IND_OBJ,
    COS_NODE_INT,
    COS_NODE_LITERAL,
    COS_NODE_NAME,
    COS_NODE_NULL,
    COS_NODE_REAL,
    COS_NODE_REF
} COS_NODE_TYPE;

typedef struct CosBlankNode {
    uint16_t type;
    uint16_t ref_count;
} CosNode;

typedef struct {
    uint16_t        type;
    uint16_t        ref_count;
    uint64_t        obj_num;
    uint32_t        gen_num;
} CosRef;

typedef struct {
    uint16_t        type;
    uint16_t        ref_count;
    uint64_t        obj_num;
    uint32_t        gen_num;
    CosNode*        value;
} CosIndObj;

typedef struct {
    uint16_t        type;
    uint16_t        ref_count;
    size_t          elems;
    CosNode**       values;
} CosArray;

typedef struct {
    uint16_t              type;
    uint16_t              ref_count;
    size_t                elems;
    CosNode**             values;
    PDF_TYPE_CODE_POINTS* keys;
    uint16_t*             key_lens;
} CosDict;

typedef struct {
    uint16_t        type;
    uint16_t        ref_count;
    PDF_TYPE_BOOL   value;
} CosBool;

typedef struct {
    uint16_t        type;
    uint16_t        ref_count;
    PDF_TYPE_INT    value;
} CosInt;

typedef struct {
    uint16_t        type;
    uint16_t        ref_count;
    PDF_TYPE_REAL   value;
} CosReal;

typedef struct CosStringyNode {
    uint16_t        type;
    uint16_t        ref_count;
    PDF_TYPE_STRING value;
} CosHexString;

typedef struct CosStringyNode CosName;
typedef struct CosStringyNode CosLiteral;
typedef struct CosBlankNode CosNull;

DLLEXPORT void cos_node_reference(CosNode*);
DLLEXPORT void cos_node_done(CosNode*);

DLLEXPORT CosRef* cos_ref_new(CosRef*, uint64_t, uint32_t);
DLLEXPORT size_t cos_ref_write(CosRef*, char*, size_t);

DLLEXPORT CosIndObj* cos_ind_obj_new(CosIndObj*, uint64_t, uint32_t, CosNode*);
DLLEXPORT size_t cos_ind_obj_write(CosIndObj*, char*, size_t);

DLLEXPORT CosInt* cos_int_new(CosInt*, PDF_TYPE_INT);
DLLEXPORT size_t cos_int_write(CosInt*, char*, size_t);

DLLEXPORT CosArray* cos_array_new(CosArray*, CosNode**, size_t);
DLLEXPORT size_t cos_array_write(CosArray*, char*, size_t);

DLLEXPORT CosDict* cos_dict_new(CosDict*, PDF_TYPE_CODE_POINTS*, CosNode**, uint16_t*, size_t);
DLLEXPORT CosNode* cos_dict_lookup(CosDict*, PDF_TYPE_CODE_POINTS, uint16_t);
DLLEXPORT size_t cos_dict_write(CosDict*, char*, size_t);

#endif
