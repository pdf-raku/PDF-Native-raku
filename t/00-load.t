use v6;
use Test;
plan 2;

use PDF::Native;
use PDF::Native::Defs :libpdf;

ok libpdf.IO.s, libpdf.IO.path ~ ' library has been built';
unless libpdf.IO.s {
    bail-out "unable to access {libpdf.basename}, has it been built, (e.g. 'zef build .' or 'raku Build.rakumod'" ~ ('Makefile'.IO.e ?? ", or 'make'" !! '') ~ ')';
}

pass 'loaded';
done-testing;
