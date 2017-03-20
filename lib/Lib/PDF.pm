use v6;

module Lib::PDF {
    use LibraryMake;
    # Find our compiled library.
    sub libpdf is export(:libpdf) {
        state $ = do {
            my $so = get-vars('')<SO>;
            ~(%?RESOURCES{"lib/libpdf$so"});
        }
    }

}
