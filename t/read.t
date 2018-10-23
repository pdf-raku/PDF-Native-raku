use v6;
use Test;
plan 3;

use Lib::PDF::Reader;

given Lib::PDF::Reader {

     enum <free inuse>;

     my uint64 @xref = (
        0, 65535, free,
        42, 0, inuse,
        69, 0, inuse,
        9000000100, 2, inuse,
     );
     my Str $xref = (
         '0000000000 65535 f ',
         '0000000042 00000 n ',
         '0000000069 00000 n ',
         '9000000100 00002 n ',
         ''
     ).join(10.chr);
     my Blob $buf = $xref.encode('latin-1');

     is-deeply .read-entries($buf,4), @xref;
     is-deeply .read-entries($buf), @xref;
     $buf = $xref.subst("69", "6x").encode('latin-1');
     @xref = (
         0, 65535, free,
         42, 0, inuse,
         0, 0, 0,
         0, 0, 0,
     );
     is-deeply .read-entries($buf), @xref;
}
