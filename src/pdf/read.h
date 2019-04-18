#ifndef PDF_READ_H_
#define PDF_READ_H_

DLLEXPORT size_t pdf_read_xref_entry_count(PDF_STRING buf, size_t buf_len);

/* read xref entries;
   typedef enum {free, inuse};
   // status, obj#, gen#,  offset
     0000000000 65535 f 
     0000000042 00000 n 
     0000000069 00000 n 
     0000000100 00002 n
   ->
   {  0,    65535, free,
      42,   0      inuse,
      60,   0      inuse,
      100,  2      inuse }
*/
DLLEXPORT size_t pdf_read_xref_seg(PDF_XREF xref, PDF_UINT length, PDF_STRING buf, size_t buf_len, size_t obj_first_num);

DLLEXPORT size_t pdf_read_xref(PDF_XREF xref, PDF_STRING buf, size_t buf_len);

#endif
