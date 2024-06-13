unit module PDF::Native::Defs;

constant libpdf is export(:libpdf) = %?RESOURCES<libraries/pdf>;

constant PDF_TYPE_BOOL   is export(:types) = int32;
constant PDF_TYPE_INT    is export(:types) = int32;
constant PDF_TYPE_INT64  is export(:types) = int64;
constant PDF_TYPE_UINT   is export(:types) = uint32;
constant PDF_TYPE_REAL   is export(:types) = num64;
constant PDF_TYPE_STRING is export(:types) = str;
constant PDF_TYPE_CODE_POINTS is export(:types) = Blob[uint32];
constant PDF_TYPE_XREF   is export(:types) = Blob[uint64];

