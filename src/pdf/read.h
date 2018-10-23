#ifndef PDF_READ_H_
#define PDF_READ_H_

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
DLLEXPORT size_t pdf_read_xref_seg(PDF_UINT64 *xref, PDF_UINT length, PDF_STRING out, size_t out_len);

#endif
