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
    COS_NODE_HEX_STR, /* 4 */
    COS_NODE_IND_OBJ,
    COS_NODE_INT,
    COS_NODE_LIT_STR, /* 7 */
    COS_NODE_NAME,
    COS_NODE_NULL,
    COS_NODE_REAL,
    COS_NODE_REF,
    COS_NODE_STREAM,
    COS_NODE_CONTENT,
    COS_NODE_OP,
    COS_NODE_OP_IMG_DATA
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
    size_t*         index;
    size_t          index_len;
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
} CosHexString, CosLiteralStr;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    CosDict*        dict;
    char*           value;
    union {
        size_t          value_len; /* buffer length, if fetched */
        size_t          value_pos; /* start position in buffer, otherwise */
    };
} CosStream;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    size_t          elems;
    CosNode**       values;
    char*           opn;
} CosOp;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    char*           value;
    size_t          value_len;
} CosOpImageData;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    size_t          elems;
    CosOp**         values;
} CosContent;

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
DLLEXPORT size_t cos_array_write(CosArray*, char*, size_t, int);

DLLEXPORT CosDict* cos_dict_new(CosDict*, CosName**, CosNode**, size_t);
DLLEXPORT size_t* cos_dict_build_index(CosDict*);
DLLEXPORT CosNode* cos_dict_lookup(CosDict*, CosName*);
DLLEXPORT size_t cos_dict_write(CosDict*, char*, size_t, int);

DLLEXPORT CosName* cos_name_new(CosName*, PDF_TYPE_CODE_POINTS, uint16_t);
DLLEXPORT size_t cos_name_write(CosName*, char*, size_t);

DLLEXPORT CosLiteralStr* cos_literal_new(CosLiteralStr*, PDF_TYPE_STRING, size_t);
DLLEXPORT size_t cos_literal_write(CosLiteralStr*, char*, size_t);

DLLEXPORT CosHexString* cos_hex_string_new(CosHexString*, PDF_TYPE_STRING, size_t);
DLLEXPORT size_t cos_hex_string_write(CosHexString*, char*, size_t);

DLLEXPORT CosNull* cos_null_new(CosNull*);
DLLEXPORT size_t cos_null_write(CosNull* self, char*, size_t);

DLLEXPORT CosStream* cos_stream_new(CosStream*, CosDict*, unsigned char*, size_t);

DLLEXPORT size_t cos_stream_write(CosStream*, char*, size_t);

DLLEXPORT CosOp* cos_op_new(CosOp*, char*, CosNode**, size_t);
DLLEXPORT size_t cos_op_write(CosOp*, char*, size_t, int);

DLLEXPORT CosContent* cos_content_new(CosContent*, CosOp**, size_t);
DLLEXPORT size_t cos_content_write(CosContent*, char*, size_t);

DLLEXPORT CosOpImageData* cos_op_image_data_new(CosOpImageData*, char*, size_t);

#endif
