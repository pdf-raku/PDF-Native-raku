use v6;

#| native implementations of PDF functions.
unit class PDF::Native:ver<0.1.4>;

use LibraryMake;
use NativeCall;
use PDF::Native::Defs :libpdf;
# Find our compiled library.

sub pdf_version
    returns Str
    is native(libpdf) {*};

method lib-version {
    return Version.new: pdf_version();
}

