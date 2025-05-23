#ifndef PDF_COS_H_
#define PDF_COS_H_

#include <stddef.h>
#include <stdint.h>
#include "pdf/types.h"

typedef enum COS_NODE_TYPE {
    COS_NODE_ANY,
    COS_NODE_ARRAY,
    COS_NODE_BOOL,
    COS_NODE_COMMENT,
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
    COS_NODE_INLINE_IMAGE
} CosNodeType;

typedef enum {
    COS_CMP_INVALID = -1,
    COS_CMP_EQUAL,
    COS_CMP_SIMILAR,
    COS_CMP_DIFFERENT,
    COS_CMP_DIFFERENT_TYPE
} CosCmpResult;

typedef enum {
    COS_OP_Other, /* Possibly valid in BX .. EX block */
    COS_OP_BeginImage, COS_OP_ImageData, COS_OP_EndImage,
    COS_OP_BeginMarkedContent, COS_OP_BeginMarkedContentDict,
    COS_OP_EndMarkedContent, COS_OP_BeginText, COS_OP_EndText,
    COS_OP_BeginExtended, COS_OP_EndExtended,
    COS_OP_CloseEOFillStroke, COS_OP_CloseFillStroke,
    COS_OP_EOFillStroke, COS_OP_FillStroke, COS_OP_CurveTo,
    COS_OP_ConcatMatrix, COS_OP_SetFillColorSpace,
    COS_OP_SetStrokeColorSpace, COS_OP_SetDashPattern,
    COS_OP_SetCharWidth, COS_OP_SetCharWidthBBox, COS_OP_XObject,
    COS_OP_MarkPointDict, COS_OP_EOFill, COS_OP_Fill,
    COS_OP_FillObsolete, COS_OP_SetStrokeGray, COS_OP_SetFillGray,
    COS_OP_SetGraphicsState, COS_OP_ClosePath, COS_OP_SetFlatness,
    COS_OP_SetLineJoin, COS_OP_SetLineCap, COS_OP_SetFillCMYK,
    COS_OP_SetStrokeCMYK, COS_OP_LineTo, COS_OP_MoveTo,
    COS_OP_SetMiterLimit, COS_OP_MarkPoint, COS_OP_EndPath,
    COS_OP_Save, COS_OP_Restore, COS_OP_Rectangle, COS_OP_SetFillRGB,
    COS_OP_SetStrokeRGB, COS_OP_SetRenderingIntent,
    COS_OP_CloseStroke, COS_OP_Stroke, COS_OP_SetStrokeColor,
    COS_OP_SetFillColor, COS_OP_SetFillColorN, COS_OP_SetStrokeColorN,
    COS_OP_ShFill, COS_OP_TextNextLine, COS_OP_SetCharSpacing,
    COS_OP_TextMove, COS_OP_TextMoveSet, COS_OP_SetFont,
    COS_OP_ShowText, COS_OP_ShowSpaceText, COS_OP_SetTextLeading,
    COS_OP_SetTextMatrix, COS_OP_SetTextRender, COS_OP_SetTextRise,
    COS_OP_SetWordSpacing, COS_OP_SetHorizScaling,
    COS_OP_CurveToInitial, COS_OP_EOClip, COS_OP_Clip,
    COS_OP_SetLineWidth, COS_OP_CurveToFinal, COS_OP_MoveSetShowText,
    COS_OP_MoveShowText
} CosOpCode;

typedef struct {
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

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    PDF_TYPE_CODE_POINTS value;
    uint16_t        value_len;
} CosName;

typedef struct CosContainerNode {
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
    size_t          elems;
    CosNode**       values;
    /* struct CosContainerNode */
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
    PDF_TYPE_INT64  value;
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
} CosHexString, CosLiteralStr, CosComment;

typedef struct CosStreamish {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    CosDict*        dict;
    char*           value;
    union {
        size_t      value_len; /* length of value when loaded */
        size_t      value_pos; /* position in input buffer otherwise */
    };
} CosStream, CosInlineImage;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    size_t          elems;
    CosNode**       values;
    /* struct CosContainerNode */
    char*           opn;
    CosOpCode       sub_type;
} CosOp;

typedef struct {
    uint8_t         type;
    uint8_t         check;
    uint16_t        ref_count;
    size_t          elems;
    CosOp**         values;
    /* struct CosContainerNode */
} CosContent;

DLLEXPORT void cos_node_reference(CosNode*);
DLLEXPORT void cos_node_done(CosNode*);

DLLEXPORT int cos_node_cmp(CosNode*, CosNode*);

DLLEXPORT CosRef* cos_ref_new(uint64_t, uint32_t);
DLLEXPORT size_t cos_ref_write(CosRef*, char*, size_t);

DLLEXPORT CosIndObj* cos_ind_obj_new(uint64_t, uint32_t, CosNode*);
DLLEXPORT size_t cos_ind_obj_write(CosIndObj*, char*, size_t);

typedef enum {
    COS_CRYPT_ALL,
    COS_CRYPT_ONLY_STRINGS,
    COS_CRYPT_ONLY_STREAMS
} CosCryptMode;

typedef struct _CosCryptNodeCtx CosCryptNodeCtx;

typedef void (*CosCryptFunc) (CosCryptNodeCtx*, PDF_TYPE_STRING, size_t);

struct  _CosCryptNodeCtx {

    unsigned char* key;
    int key_len;

    CosCryptMode mode;
    CosCryptFunc crypt_cb;

    unsigned char* buf;
    size_t   buf_len;

    uint64_t obj_num;
    uint32_t gen_num;

};

DLLEXPORT CosCryptNodeCtx* cos_crypt_ctx_new(CosCryptFunc, CosCryptMode, unsigned char*, int);
DLLEXPORT void cos_crypt_ctx_done(CosCryptNodeCtx*);

DLLEXPORT void cos_ind_obj_crypt(CosIndObj*, CosCryptNodeCtx*);

DLLEXPORT CosInt* cos_int_new(PDF_TYPE_INT);
DLLEXPORT size_t cos_int_write(CosInt*, char*, size_t);

DLLEXPORT CosBool* cos_bool_new(PDF_TYPE_BOOL value);
DLLEXPORT size_t cos_bool_write(CosBool* self, char* out, size_t out_len);

DLLEXPORT CosReal* cos_real_new(PDF_TYPE_REAL value);
DLLEXPORT size_t cos_real_write(CosReal* self, char* out, size_t out_len);

DLLEXPORT CosArray* cos_array_new(CosNode**, size_t);
DLLEXPORT size_t cos_array_write(CosArray*, char*, size_t, int);

DLLEXPORT CosDict* cos_dict_new(CosName**, CosNode**, size_t);
DLLEXPORT size_t* cos_dict_build_index(CosDict*);
DLLEXPORT CosNode* cos_dict_lookup(CosDict*, CosName*);
DLLEXPORT size_t cos_dict_write(CosDict*, char*, size_t, int);

DLLEXPORT CosName* cos_name_new(PDF_TYPE_CODE_POINTS, uint16_t);
DLLEXPORT size_t cos_name_write(CosName*, char*, size_t);

DLLEXPORT CosLiteralStr* cos_literal_new(PDF_TYPE_STRING, size_t);
DLLEXPORT size_t cos_literal_write(CosLiteralStr*, char*, size_t);

DLLEXPORT CosHexString* cos_hex_string_new(PDF_TYPE_STRING, size_t);
DLLEXPORT size_t cos_hex_string_write(CosHexString*, char*, size_t);

DLLEXPORT CosNull* cos_null_new(void);
DLLEXPORT size_t cos_null_write(CosNull*, char*, size_t);

DLLEXPORT CosStream* cos_stream_new(CosDict*, unsigned char*, size_t);
DLLEXPORT int cos_stream_attach_data(CosStream*, unsigned char* , size_t, size_t);
DLLEXPORT size_t cos_stream_write(CosStream*, char*, size_t);

DLLEXPORT CosOp* cos_op_new(char*, int, CosNode**, size_t);
DLLEXPORT int cos_op_is_valid(CosOp*);
DLLEXPORT size_t cos_op_write(CosOp*, char*, size_t, int);

DLLEXPORT CosContent* cos_content_new(CosOp**, size_t);
DLLEXPORT size_t cos_content_write(CosContent*, char*, size_t);

DLLEXPORT CosInlineImage* cos_inline_image_new(CosDict* dict, unsigned char* value, size_t value_len);
DLLEXPORT size_t cos_inline_image_write(CosInlineImage*, char*, size_t, int);

DLLEXPORT CosComment* cos_comment_new(PDF_TYPE_STRING, size_t);
DLLEXPORT size_t cos_comment_write(CosComment*, char*, size_t, int);

DLLEXPORT size_t cos_node_get_write_size(CosNode*, int);

#endif
