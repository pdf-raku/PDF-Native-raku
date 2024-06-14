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
    COS_NODE_HEX_STRING,
    COS_NODE_LITERAL,
    COS_NODE_NAME,
    COS_NODE_NULL,
    COS_NODE_REAL,
    COS_NODE_REFERENCE
} COS_NODE_TYPE;

typedef struct {
    uint8_t type;
} *CosNode;

typedef struct {
    uint8_t         type;
    uint64_t        obj_num;
    uint32_t        gen_num;
    CosNode         value;
} CosRef;

typedef struct {
    uint8_t         type;
    size_t          len;
    CosNode         *value;
} CosArray;

typedef struct {
    uint8_t         type;
    size_t          len;
    CosNode         *key;
    CosNode         *value;
} CosDict;

typedef struct {
    uint8_t         type;
    PDF_TYPE_BOOL   value;
} CosBool;

typedef struct {
    uint8_t         type;
    PDF_TYPE_REAL   value;
} CosReal;

typedef struct {
    uint8_t         type;
    PDF_TYPE_STRING value;
} CosLiteral;

typedef struct {
    uint8_t         type;
    PDF_TYPE_STRING value;
} CosHexString;

typedef struct {
    uint8_t         type;
    PDF_TYPE_STRING value;
} CosName;

typedef struct {
    uint8_t         type;
} CosNull;

#endif
