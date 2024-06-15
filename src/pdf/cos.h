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
    uint8_t type;
    uint8_t ref;
} CosNode;

typedef struct {
    uint8_t         type;
    uint8_t         ref;
    uint64_t        obj_num;
    uint32_t        gen_num;
} CosRef;

typedef struct {
    uint8_t         type;
    uint8_t         ref;
    uint64_t        obj_num;
    uint32_t        gen_num;
    CosNode*        value;
} CosIndObj;

typedef struct {
    uint8_t         type;
    uint8_t         ref;
    size_t          len;
    CosNode*        value;
} CosArray;

typedef struct {
    uint8_t         type;
    uint8_t         ref;
    size_t          len;
    char*           key;
    CosNode*        value;
} CosDict;

typedef struct {
    uint8_t         type;
    uint8_t         ref;
    PDF_TYPE_BOOL   value;
} CosBool;

typedef struct {
    uint8_t         type;
    uint8_t         ref;
    PDF_TYPE_INT    value;
} CosInt;

typedef struct {
    uint8_t         type;
    uint8_t         ref;
    PDF_TYPE_REAL   value;
} CosReal;

typedef struct CosStringyNode {
    uint8_t         type;
    uint8_t         ref;
    PDF_TYPE_STRING value;
} CosHexString;

typedef struct CosStringyNode CosName;
typedef struct CosStringyNode CosLiteral;
typedef struct CosBlankNode CosNull;

DLLEXPORT void cos_node_done(CosNode*);

DLLEXPORT CosRef* cos_ref_new(CosRef*, uint64_t, uint32_t);
DLLEXPORT size_t cos_ref_write(CosRef*, char*, size_t);

DLLEXPORT CosIndObj* cos_ind_obj_new(CosIndObj*, uint64_t, uint32_t, CosNode*);
DLLEXPORT size_t cos_ind_obj_write(CosIndObj*, char*, size_t);

DLLEXPORT CosInt* cos_int_new(CosInt*, PDF_TYPE_INT);
DLLEXPORT size_t cos_int_write(CosInt*, char*, size_t);

#endif
