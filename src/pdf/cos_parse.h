#ifndef COS_PARSE_H_
#define COS_PARSE_H_

typedef enum {
    COS_PARSE_NIBBLE,
    COS_PARSE_SCAN,
    COS_PARSE_REPAIR
} CosParseMode;

DLLEXPORT CosIndObj* cos_parse_ind_obj(char*, size_t, CosParseMode);
DLLEXPORT CosNode* cos_parse_obj(char *, size_t);
DLLEXPORT CosContent* cos_parse_content(char* in_buf, size_t in_len);

#endif
