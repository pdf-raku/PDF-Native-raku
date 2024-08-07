#ifndef PDF_WRITE_H_
#define PDF_WRITE_H_

/* write boolean: 0 -> "true", 1 -> "false" */
DLLEXPORT size_t pdf_write_bool(PDF_TYPE_BOOL, char *out, size_t out_len);

/* write integer: 42 -> "42" */
DLLEXPORT size_t pdf_write_int(PDF_TYPE_INT64, char *out, size_t out_len);

/* write real: pi -> "3.14159" */
DLLEXPORT size_t pdf_write_real(PDF_TYPE_REAL, char *out, size_t out_len);

/* write literal string: "Hi\nthere" -> "(Hi\nthere)" */
DLLEXPORT size_t pdf_write_literal(PDF_TYPE_STRING, size_t in_len, char* out, size_t out_len);

/* write hex string: "snoopy" -> "<736e6f6f7079>" */
DLLEXPORT size_t pdf_write_hex_string(PDF_TYPE_STRING, size_t in_len, char* out, size_t out_len);

/* write comment: "comment1\ncomment2" -> "% comment1\n% comment2" */
DLLEXPORT size_t pdf_write_comment(PDF_TYPE_STRING , size_t in_len, char* out, size_t out_len);

DLLEXPORT int pdf_write_name_code(PDF_TYPE_CODE_POINT cp, char*, size_t); 

/* write name: "Hi#there" -> "/Hi##there" */
DLLEXPORT size_t pdf_write_name(PDF_TYPE_CODE_POINTS, size_t in_len, char* out, size_t out_len);

/* write xref entries;
   typedef enum {free, inuse};
   // status, obj#, gen#,  offset
   {  0,    65535, free,
      42,   0      inuse,
      60,   0      inuse,
      100,  2      inuse }
   ->
     0000000000 65535 f 
     0000000042 00000 n 
     0000000069 00000 n 
     0000000100 00002 n
*/
DLLEXPORT size_t pdf_write_xref_seg(PDF_TYPE_XREF, PDF_TYPE_UINT length, PDF_TYPE_STRING buf, size_t buf_len);

#endif
