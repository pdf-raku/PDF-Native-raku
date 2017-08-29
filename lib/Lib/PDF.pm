use v6;

module Lib::PDF {
    use LibraryMake;
    use NativeCall;
    # Find our compiled library.
    sub libpdf is export(:libpdf) {
        state $ = do {
            my $so = get-vars('')<SO>;
            ~(%?RESOURCES{"lib/libpdf$so"});
        }
    }

    constant PDF_BOOL   is export(:types) = int32;
    constant PDF_INT    is export(:types) = int32;
    constant PDF_UINT   is export(:types) = uint32;
    constant PDF_UINT64 is export(:types) = uint64;
    constant PDF_REAL   is export(:types) = num64;
    constant PDF_STRING is export(:types) = str;
}
