#ifndef PDF__BUFCAT_H_
#define PDF__BUFCAT_H_

static int _bufcat(char* out, int out_len, char *in) {
    int n;
    for (n=0; in[n]; n++) {
        if (out_len-- <= 0) return 0;
        out[n] = in[n];
    }
    return n;
}

#endif
