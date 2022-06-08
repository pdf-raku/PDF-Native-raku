use v6;

#| native implementations of PDF functions.
class PDF::Native:ver<0.0.2> {
    use LibraryMake;
    use NativeCall;
    # Find our compiled library.
    constant libpdf is export(:libpdf) = %?RESOURCES<libraries/pdf>;

    sub pdf_version
        returns Str
        is native(libpdf) {*};

    constant PDF_BOOL   is export(:types) = int32;
    constant PDF_INT    is export(:types) = int32;
    constant PDF_UINT   is export(:types) = uint32;
    constant PDF_UINT64 is export(:types) = uint64;
    constant PDF_REAL   is export(:types) = num64;
    constant PDF_STRING is export(:types) = str;
    constant PDF_CODE_POINTS is export(:types) = Blob[uint32];
    constant PDF_XREF   is export(:types) = Blob[uint64];

    method lib-version {
        return Version.new: pdf_version();
    }
}
