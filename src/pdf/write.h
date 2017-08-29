#ifndef PDF_WRITE_H_
#define PDF_WRITE_H_

/* write boolean: 0 -> "true", 1 -> "false" */
DLLEXPORT uint8_t* pdf_write_bool(PDF_BOOL val, uint8_t *out, size_t out_len);

/* write integer: 42 -> "42" */
DLLEXPORT uint8_t* pdf_write_int(PDF_INT val, uint8_t *out, size_t out_len);

/* write real: pi -> "3.14159" */
DLLEXPORT uint8_t* pdf_write_real(PDF_REAL val, uint8_t *out, size_t out_len);

/* write literal string: "Hi\nthere" -> "(Hi\nthere)" */
DLLEXPORT uint8_t* pdf_write_literal(PDF_STRING val, size_t in_len, PDF_STRING out, size_t out_len);

/* write hex string: "snoopy" -> "<736e6f6f7079>" */
DLLEXPORT uint8_t* pdf_write_hex_string(PDF_STRING val, size_t in_len, PDF_STRING out, size_t out_len);

/* write name: "Hi#there" -> "/Hi##there" */
DLLEXPORT uint8_t* pdf_write_name(uint32_t *val, size_t in_len, PDF_STRING out, size_t out_len);

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
DLLEXPORT uint8_t* pdf_write_entries(PDF_UINT64 *xref, PDF_UINT length, uint8_t *out, size_t out_len);

#endif
