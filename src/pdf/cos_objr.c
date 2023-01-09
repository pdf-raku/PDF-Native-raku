#include "pdf.h"
#include "cos.h"
#include "pdf/write.h"
#include "pdf/cos_objr.h"
#include <string.h>

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

DLLEXPORT int pdf_cos_objr_write(cos_objr objr, char* out, int out_len) {
    char *start = out;
    char *end = out + out_len;

    if (out && out_len && objr) {
        strcpy(out, "<< /Type /OBJR");
        out += strnlen(out, out_len);

        if (objr->Pg && objr->Pg->obj_num) {
            out += _entry("Pg", out, end - out);
            out += _pad(out, end - out);
            out += pdf_cos_indref_write(objr->Pg, out, end - out);
        }

        if (objr->Obj && objr->Obj->obj_num) {
            cos_indref Obj = (cos_indref) objr->Obj;
            out += _entry("Obj", out, end - out);
            out += _pad(out, end - out);
            out += pdf_cos_indref_write(Obj, out, end - out);
        }

        strncpy(out, " >>", end - out);
        out += strnlen(out, out_len);
        return out - start;
    }
    else {
        return 0;
    }
}
