#include "pdf.h"
#include "cos.h"
#include "pdf/write.h"
#include <string.h>

struct _cos_struct_elem {
    int32_t obj_num;
    int16_t gen_num;
    char* Type;
    char* S;
    char* ID;
    cos_indref Pg;
};

typedef struct _cos_struct_elem *cos_struct_elem;

static size_t _entry(char *key, char *out, size_t out_len) {
    snprintf(out, out_len, " /%s", key);
    return strnlen(out, out_len);
}

static size_t _pad(char *out, size_t out_len) {
    if (out_len > 1) {
        *out = ' ';
        return 1;
    }
    else {
        return 0;
    }
}


DLLEXPORT int pdf_cos_struct_elem_write(cos_struct_elem elem, char* out, int out_len) {
    char *start = out;
    char *end = out + out_len;

    if (out && out_len && elem) {
        strcpy(out, "<< /Type /StructElem");
        out += strnlen(out, out_len);
        if (elem->S && *(elem->S)) {
            out += _entry("S", out, end - out);
            out += _entry(elem->S, out, end - out);
        }
        if (elem->ID && *(elem->ID)) {
            out += _entry("ID", out, end - out);
            out += _pad(out, end - out);
            out += pdf_write_hex_string(elem->ID, strlen(elem->ID), out, end - out);
        }
        if (elem->Pg && elem->Pg->obj_num) {
            out += _entry("Pg", out, end - out);
            out += _pad(out, end - out);
            out += pdf_cos_indref_write(elem->Pg, out, end - out);
        }
        strncpy(out, " >>", end - out);
        out += strnlen(out, out_len);
        return out - start;
    }
    else {
        return 0;
    }
}
