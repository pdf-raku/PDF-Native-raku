#include "pdf.h"
#include "cos.h"
#include "pdf/write.h"
#include "pdf/cos_mcr.h"
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


DLLEXPORT int pdf_cos_mcr_write(cos_mcr mcr, char* out, int out_len) {
    char *start = out;
    char *end = out + out_len;

    if (out && out_len && mcr) {
        strcpy(out, "<< /Type /MCR");
        out += strnlen(out, out_len);
        if (mcr->Pg && mcr->Pg->obj_num) {
            out += _entry("Pg", out, end - out);
            out += _pad(out, end - out);
            out += pdf_cos_indref_write(mcr->Pg, out, end - out);
        }
        if (mcr->Stm && mcr->Stm->obj_num) {
            cos_indref Stm = (cos_indref) mcr->Stm;
            out += _entry("Stm", out, end - out);
            out += _pad(out, end - out);
            out += pdf_cos_indref_write(Stm, out, end - out);
        }

        out +=  _entry("MCID", out, end - out);
        out += _pad(out, end - out);
        out += pdf_write_int(mcr->MCID, out, end - out);

        strncpy(out, " >>", end - out);
        out += strnlen(out, out_len);
        return out - start;
    }
    else {
        return 0;
    }
}
