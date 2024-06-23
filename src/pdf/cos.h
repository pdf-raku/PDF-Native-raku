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
    COS_NODE_HEX, /* 4 */
    COS_NODE_IND_OBJ,
    COS_NODE_INT,
    COS_NODE_LITERAL, /* 7 */
    COS_NODE_NAME,
    COS_NODE_NULL,
    COS_NODE_REAL,
    COS_NODE_REF,
    COS_NODE_STREAM
} CosNodeType;

typedef enum {
    COS_CMP_EQUAL,
    COS_CMP_SIMILAR,
    COS_CMP_DIFFERENT,
    COS_CMP_DIFFERENT_TYPE
} CosCmpResult;

typedef struct CosBlankNode {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
} CosNode, CosNull;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    uint64_t        obj_num;
    uint32_t        gen_num;
} CosRef;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    uint64_t        obj_num;
    uint32_t        gen_num;
    CosNode*        value;
} CosIndObj;

typedef struct CosArrayishNode {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    size_t          elems;
    CosNode**       values;
} CosArray;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    PDF_TYPE_CODE_POINTS value;
    uint16_t        value_len;
} CosName;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    size_t          elems;
    CosNode**       values;
    CosName**       keys;
} CosDict;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    PDF_TYPE_BOOL   value;
} CosBool;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    PDF_TYPE_INT    value;
} CosInt;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    PDF_TYPE_REAL   value;
} CosReal;

typedef struct CosStringyNode {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    PDF_TYPE_STRING value;
    size_t          value_len;
} CosHexString, CosLiteral;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    CosDict*        dict;
    char*           value;
    size_t          value_len;
} CosStream;

DLLEXPORT void cos_node_reference(CosNode*);
DLLEXPORT void cos_node_done(CosNode*);

DLLEXPORT int cos_node_cmp(CosNode*, CosNode*);

DLLEXPORT CosRef* cos_ref_new(CosRef*, uint64_t, uint32_t);
DLLEXPORT size_t cos_ref_write(CosRef*, char*, size_t);

DLLEXPORT CosIndObj* cos_ind_obj_new(CosIndObj*, uint64_t, uint32_t, CosNode*);
DLLEXPORT size_t cos_ind_obj_write(CosIndObj*, char*, size_t);

typedef enum {
    COS_CRYPT_ALL,
    COS_CRYPT_ONLY_STRINGS,
    COS_CRYPT_ONLY_STREAMS
} CosCryptMode;

typedef struct _CosCryptNodeCtx CosCryptNodeCtx;

typedef void (*CosCryptFunc) (CosCryptNodeCtx*, PDF_TYPE_STRING, size_t);

struct  _CosCryptNodeCtx {

    unsigned char *key;
    int key_len;

    CosCryptMode mode;
    CosCryptFunc crypt_cb;

    unsigned char *buf;
    size_t   buf_len;

    uint64_t obj_num;
    uint32_t gen_num;

};

DLLEXPORT CosCryptNodeCtx* cos_crypt_ctx_new(CosCryptNodeCtx*, CosCryptFunc, CosCryptMode, unsigned char*, int);
DLLEXPORT void cos_crypt_ctx_done(CosCryptNodeCtx*);

DLLEXPORT void cos_ind_obj_crypt(CosIndObj*, CosCryptNodeCtx*);

DLLEXPORT CosInt* cos_int_new(CosInt*, PDF_TYPE_INT);
DLLEXPORT size_t cos_int_write(CosInt*, char*, size_t);

DLLEXPORT CosBool* cos_bool_new(CosBool* self, PDF_TYPE_BOOL value);
DLLEXPORT size_t cos_bool_write(CosBool* self, char* out, size_t out_len);

DLLEXPORT CosReal* cos_real_new(CosReal* self, PDF_TYPE_REAL value);
DLLEXPORT size_t cos_real_write(CosReal* self, char* out, size_t out_len);

DLLEXPORT CosArray* cos_array_new(CosArray*, CosNode**, size_t);
DLLEXPORT size_t cos_array_write(CosArray*, char*, size_t);

DLLEXPORT CosDict* cos_dict_new(CosDict*, CosName**, CosNode**, size_t);
DLLEXPORT CosNode* cos_dict_lookup(CosDict*, PDF_TYPE_CODE_POINTS, uint16_t);
DLLEXPORT size_t cos_dict_write(CosDict*, char*, size_t);

DLLEXPORT CosName* cos_name_new(CosName*, PDF_TYPE_CODE_POINTS, uint16_t);
DLLEXPORT size_t cos_name_write(CosName*, char*, size_t);

DLLEXPORT CosLiteral* cos_literal_new(CosLiteral*, PDF_TYPE_STRING, size_t);
DLLEXPORT size_t cos_literal_write(CosLiteral*, char*, size_t);

DLLEXPORT CosHexString* cos_hex_string_new(CosHexString*, PDF_TYPE_STRING, size_t);
DLLEXPORT size_t cos_hex_string_write(CosHexString*, char*, size_t);

DLLEXPORT CosNull* cos_null_new(CosNull*);
DLLEXPORT size_t cos_null_write(CosNull* self, char*, size_t);

DLLEXPORT CosIndObj* cos_ind_obj_new(CosIndObj*, uint64_t, uint32_t, CosNode*);

DLLEXPORT CosStream* cos_stream_new(CosStream*, CosDict*, unsigned char*, size_t);

DLLEXPORT size_t cos_stream_write(CosStream*, char*, size_t);

#endif
